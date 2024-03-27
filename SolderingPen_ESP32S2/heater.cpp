#include "heater.hpp"
#include <array>
#include "log.h"
#include "const.h"
#include "main.h"

#define HEATER_TASK_PRIO          tskIDLE_PRIORITY+1    // task priority
#ifdef PTS200_DEBUG_LEVEL
#define HEATER_TASK_STACK         2048                  // sprintf could take lot's of stack mem for debug messages
#else
#define HEATER_TASK_STACK         1024
#endif
#define HEATER_TASK_NAME          "HEATER"

#define LEDC_MODE       LEDC_LOW_SPEED_MODE             // S2 supports only low speed mode
#define LEDC_DUTY_RES   HEATER_RES
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_FREQUENCY  HEATER_FREQ

#define HEATER_OPAMP_STABILIZE_MS 5                     // how long to wait after disabling PWM to let OpAmp stabilize

#define HEATER_ADC_SAMPLES        8                     // number of ADC reads to averate tip tempearture

// normal interval between temp measurments
constexpr TickType_t measure_delay_ticks = pdMS_TO_TICKS(1000 / HEATER_MEASURE_RATE);
// long interval between temp measurments
constexpr TickType_t long_measure_delay_ticks = pdMS_TO_TICKS(500);
// idle interval between temp measurments
constexpr TickType_t idle_delay_ticks = pdMS_TO_TICKS(1000);



void TipHeater::init(){
    // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {
      .speed_mode       = LEDC_MODE,
      .duty_resolution  = LEDC_DUTY_RES,        
      .timer_num        = LEDC_TIMER,
      .freq_hz          = LEDC_FREQUENCY,
      .clk_cfg          = LEDC_AUTO_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {
      .gpio_num       = _gpio,
      .speed_mode     = LEDC_MODE,
      .channel        = _ledc_channel,
      .intr_type      = LEDC_INTR_DISABLE,
      .timer_sel      = LEDC_TIMER,
      .duty           = 0,
      .hpoint         = 0,
      .flags          = {.output_invert  = _invert }
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  adc_sensor.attach(SENSOR_PIN);

  // set PID output range
  _pid.setOutputRange(0, 1<<HEATER_RES);

  // create RTOS task
  _start_runner();
}


TipHeater::~TipHeater(){
  ledc_stop(LEDC_MODE, _ledc_channel, _invert);
  if(_task_hndlr)
    vTaskDelete(_task_hndlr);
  _task_hndlr = nullptr;
}


void TipHeater::_start_runner(){
  if (_task_hndlr) return;    // we are already running
  xTaskCreatePinnedToCore(TipHeater::_runner,
                          HEATER_TASK_NAME,
                          HEATER_TASK_STACK,
                          (void *)this,
                          HEATER_TASK_PRIO,
                          &_task_hndlr,
                          tskNO_AFFINITY ) == pdPASS;
}

void TipHeater::_runnerHndlr(){
  TickType_t xLastWakeTime = xTaskGetTickCount ();
  TickType_t delay_time = measure_delay_ticks;
  for (;;){
    // sleep to accomodate specified measuring rate
    // if task has been delayed, than we can't keep up with desired measure rate, let's give other tasks time to run anyway
    if ( xTaskDelayUntil( &xLastWakeTime, delay_time ) ) taskYIELD();

    if (ledc_get_duty(LEDC_MODE, _ledc_channel)){
      // shut off heater in order to measure temperature 关闭加热器以测量温度
      ledc_set_duty(LEDC_MODE, _ledc_channel, 0);
      ledc_update_duty(LEDC_MODE, _ledc_channel);
      // idle while OpAmp stabilizes
      vTaskDelay(pdMS_TO_TICKS(HEATER_OPAMP_STABILIZE_MS));
    }

    auto t = _denoiseADC();

    // skip cycle if heater is disabled or tip is missing
    if (!enabled || t > TEMP_NOTIP){
      _t.calibrated = t;
      _t.avg = t;
      // set pwm to 0 if active
      if (ledc_get_duty(LEDC_MODE, _ledc_channel)){
        ledc_set_duty(LEDC_MODE, _ledc_channel, 0);
        ledc_update_duty(LEDC_MODE, _ledc_channel);
      }
      delay_time = idle_delay_ticks;
      continue;
    }

    // read tip temperature and average it with previous readings
    _t.avg += (t - _t.avg) * SMOOTHIE;  // stabilize ADC temperature reading 稳定ADC温度读数
    // calibrate temp based on map table
    // note: this is ugly external function, I will rework it later
    _t.calibrated = calculateTemp(_t.avg);
    LOGV(T_HEAT, printf, "avg T: %5.1f, calibrated T: %d\n", _t.avg, _t.calibrated);

    auto diff = abs(_t.target - _t.calibrated);
    // if PID algo should be engaged
    if (diff < PID_ENGAGE_DIFF){
      _pwm_duty = _pid.step(_t.target, _t.calibrated);
      delay_time = measure_delay_ticks;
    } else {
      // heater must be either turned on or off
      _t.calibrated < _t.target ? _pwm_duty = 1<<HEATER_RES : 0;
      _pid.clear();
      delay_time = long_measure_delay_ticks;
    }

    ledc_set_duty(LEDC_MODE, _ledc_channel, _pwm_duty);
    ledc_update_duty(LEDC_MODE, _ledc_channel);
    LOGV(T_PWM, printf, "Duty:%u\n", _pwm_duty);
  }
  // Task must self-terminate (if ever)
  vTaskDelete(NULL);
}

void TipHeater::enable(){
  enabled = true;
  LOGI(T_PWM, printf, "Enable heater, target T:%d\n", _t.target);
};

void TipHeater::disable(){
  if (enabled){
    enabled = false;
    ledc_set_duty(LEDC_MODE, _ledc_channel, 0);
    ledc_update_duty(LEDC_MODE, _ledc_channel);
    LOGI(T_PWM, println, "Disable heater");
  }
}

// 对32个ADC读数进行平均以降噪
//  VP+_Ru = 100k, Rd_GND = 1K
float TipHeater::_denoiseADC(){
  std::array<uint32_t, 8> samples;

  for (auto &i : samples){
    i = adc_sensor.readMiliVolts();
    // value = constrain(0.4432 * raw_adc + 29.665, 20, 1000);
  }

  // sort resultArray with low time complexity
  // 用低时间复杂度对resultArray进行排序
  for (size_t i = 0; i < samples.size(); ++i){
    for (size_t j = i + 1; j < samples.size(); ++j){
      if (samples[i] > samples[j]) {
        auto temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }

  uint32_t result{0};
  // get the average of the middle 4 readings 获取中间20个读数的平均值
  for (uint8_t i = 2; i < 6; i++) {
    result += samples[i];
  }

  // convert mV to Celsius
  auto t = constrain(0.5378 * result / 4 + 6.3959, 20.0, 1000.0);
  LOGV(T_ADC, printf, "avg mV:%u / Temp C:%6.1f\n", result / 4, t);
////  resultArray[i] = constrain(0.5378 * raw_adc + 6.3959, 20, 1000); // y = 0.5378x + 6.3959;
  return t;
}

