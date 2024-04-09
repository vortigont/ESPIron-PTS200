#include "heater.hpp"
#include <array>
#include "evtloop.hpp"
#include "const.h"
#include "main.h"
#include "log.h"

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

TipHeater::~TipHeater(){
  // unsubscribe from event bus
  if (_evt_cmd_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_SET_EVT, ESP_EVENT_ANY_ID, _evt_cmd_handler);
    _evt_cmd_handler = nullptr;
  }

  if (_evt_ntf_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_NOTIFY, ESP_EVENT_ANY_ID, _evt_ntf_handler);
    _evt_ntf_handler = nullptr;
  }

  _stop_runner();
}

void TipHeater::init(){
  adc_sensor.attach(SENSOR_PIN);

  // set PID output range
  _pid.setOutputRange(0, 1<<HEATER_RES);

  // event bus subscriptions
  if (!_evt_cmd_handler){
    esp_event_handler_instance_register_with(
      evt::get_hndlr(),
      IRON_SET_EVT,
      ESP_EVENT_ANY_ID,    // subscribe to 'sensorsReload command'
      [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<TipHeater*>(self)->_evt_picker(base, id, data); },
      this,
      &_evt_cmd_handler
    );
  }

  if (!_evt_ntf_handler){
    esp_event_handler_instance_register_with(
      evt::get_hndlr(),
      IRON_NOTIFY,
      ESP_EVENT_ANY_ID,    // subscribe to 'sensorsReload command'
      [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<TipHeater*>(self)->_evt_picker(base, id, data); },
      this,
      &_evt_ntf_handler
    );
  }

  // create RTOS task that controls heater PWM
  _start_runner();
}

void TipHeater::_evt_picker(esp_event_base_t base, int32_t id, void* data){

  switch (static_cast<evt::iron_t>(id)){
    case evt::iron_t::heaterTargetT : {
      // change heater temp
      setTargetTemp(*reinterpret_cast<int32_t*>(data));
      LOGD(T_HEAT, printf, "set target T:%d\n", _t.target);
      return;
    }

    case evt::iron_t::stateWorking : {
      // enable heater
      enable();
      return;
    }

    case evt::iron_t::stateIdle : {
      // disable heater in standby mode
      disable();
      return;
    }

    case evt::iron_t::stateSuspend : {
      // disable worker task in standby mode
      _stop_runner();
      return;
    }
  }
}


void TipHeater::_start_runner(){
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
      .gpio_num       = _pwm.gpio,
      .speed_mode     = LEDC_MODE,
      .channel        = _pwm.channel,
      .intr_type      = LEDC_INTR_DISABLE,
      .timer_sel      = LEDC_TIMER,
      .duty           = 0,
      .hpoint         = 0,
      .flags          = {.output_invert  = _pwm.invert }
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  if (_task_hndlr) return;    // we are already running
  xTaskCreatePinnedToCore(TipHeater::_runner,
                          HEATER_TASK_NAME,
                          HEATER_TASK_STACK,
                          (void *)this,
                          HEATER_TASK_PRIO,
                          &_task_hndlr,
                          tskNO_AFFINITY ) == pdPASS;
}

void TipHeater::_stop_runner(){
  _state = HeaterState_t::shutoff;
  ledc_stop(LEDC_MODE, _pwm.channel, _pwm.invert);
  if(_task_hndlr)
    vTaskDelete(_task_hndlr);
  _task_hndlr = nullptr;
}

void TipHeater::_heaterControl(){
  TickType_t xLastWakeTime = xTaskGetTickCount ();
  TickType_t delay_time = measure_delay_ticks;
  for (;;){
    // sleep to accomodate specified measuring rate
    // if task has been delayed, than we can't keep up with desired measure rate, let's give other tasks time to run anyway
    if ( xTaskDelayUntil( &xLastWakeTime, delay_time ) ) taskYIELD();

    switch (_state){
      case HeaterState_t::notip :
      case HeaterState_t::inactive :
      case HeaterState_t::fault :
        delay_time = idle_delay_ticks;
        break;
      case HeaterState_t::active : {
        if (ledc_get_duty(LEDC_MODE, _pwm.channel)){
          // shut off heater in order to measure temperature 关闭加热器以测量温度
          ledc_set_duty(LEDC_MODE, _pwm.channel, 0);
          ledc_update_duty(LEDC_MODE, _pwm.channel);
          // idle while OpAmp stabilizes
          vTaskDelay(pdMS_TO_TICKS(HEATER_OPAMP_STABILIZE_MS));
        }
        break;
      }
      case HeaterState_t::shutoff :
        // how did we ended up here???
        LOGE(T_HEAT, println, "thread end up in a wrong state, aborting...");
        _task_hndlr = nullptr;
        vTaskDelete(NULL);
        return;
    }

    // measure tip temperature
    auto t = _denoiseADC();

    // check if we've lost the Tip
    if (_state != HeaterState_t::notip && t > TEMP_NOTIP){
      // we have just lost connection with a tip sensor
      // disable PWM
      ledc_set_duty(LEDC_MODE, _pwm.channel, 0);
      ledc_update_duty(LEDC_MODE, _pwm.channel);
      _state = HeaterState_t::notip;
      LOGI(T_HEAT, printf, "Iron Tip ejected, T:%d\n", t);
      EVT_POST(SENSOR_DATA, e2int(evt::iron_t::tipEject));
      continue;
    }

    // check if we've get the Tip back
    if (_state == HeaterState_t::notip && t < TEMP_NOTIP){
      _state = HeaterState_t::inactive;
      EVT_POST(SENSOR_DATA, e2int(evt::iron_t::tipInsert));
      LOGI(T_HEAT, println, "Iron Tip inserted");
      continue;
    }

    // if heater is inactive, just reset avg temperature readings and suspend
    if (_state == HeaterState_t::inactive){
      //_t.calibrated = calculateTemp(t);
      _t.calibrated = t;
      _t.avg = t;
      EVT_POST_DATA(SENSOR_DATA, e2int(evt::iron_t::tiptemp), &_t.calibrated, sizeof(_t.calibrated));
      continue;
    }

    // TODO: check for fault-overheating here

    // OK, now we are in active state for sure

    // read tip temperature and average it with previous readings
    _t.avg += (t - _t.avg) * SMOOTHIE;  // stabilize ADC temperature reading 稳定ADC温度读数
    // calibrate temp based on map table
    // note: this is ugly external function, I will rework it later
    //_t.calibrated = calculateTemp(_t.avg);
    _t.calibrated = _t.avg;
    LOGV(T_HEAT, printf, "avg T: %5.1f, calibrated T: %d\n", _t.avg, _t.calibrated);
    EVT_POST_DATA(SENSOR_DATA, e2int(evt::iron_t::tiptemp), &_t.calibrated, sizeof(_t.calibrated));

    auto diff = abs(_t.target - _t.calibrated);
    // if PID algo should be engaged
    if (diff < PID_ENGAGE_DIFF){
      _pwm.duty = _pid.step(_t.target, _t.calibrated);
      delay_time = measure_delay_ticks;
    } else {
      // heater must be either turned on or off
      _pwm.duty = _t.calibrated < _t.target ? 1<<HEATER_RES : 0;
      _pid.clear();   // reset pid algo
      // give heater more time to gain/loose temperature
      delay_time = long_measure_delay_ticks;
    }

    ledc_set_duty(LEDC_MODE, _pwm.channel,  _pwm.duty);
    ledc_update_duty(LEDC_MODE, _pwm.channel);
    PWM_LOGV(printf, "Duty:%u\n",  _pwm.duty);
  }
  // Task must self-terminate (if ever)
  vTaskDelete(NULL);
}

void TipHeater::enable(){

  switch (_state){
    case HeaterState_t::fault :
    case HeaterState_t::notip :
      // we won't recover from a faulted state, iron must be powercycled,
      // in case if no Tip is present, Heater will auto-recover once a Tip sensor is detected again
      return;
    case HeaterState_t::shutoff :
    case HeaterState_t::inactive :
      _state = HeaterState_t::active;
      if (!_task_hndlr)
        _start_runner();
    default:
      return;
  }

  LOGI(T_HEAT, printf, "Enabling, target T:%d\n", _t.target);
};

void TipHeater::disable(){
  switch (_state){
    case HeaterState_t::active :
      ledc_set_duty(LEDC_MODE, _pwm.channel, 0);
      ledc_update_duty(LEDC_MODE, _pwm.channel);
      _state = HeaterState_t::inactive;
      LOGI(T_PWM, println, "Disable heater");
      break;
    // in all other cases this call could be ignored
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
  ADC_LOGV(printf, "avg mV:%u / Temp C:%6.1f\n", result / 4, t);
////  resultArray[i] = constrain(0.5378 * raw_adc + 6.3959, 20, 1000); // y = 0.5378x + 6.3959;
  return t;
}

/*
// calculates real temperature value according to ADC reading and calibration
// values 根据ADC读数和校准值，计算出真实的温度值
int32_t calculateTemp(float t) {
//  if (RawTemp < 200)
//    CurrentTemp = map(RawTemp, 20, 200, 20, CalTemp[CurrentTip][0]);
  if (t < 200)
    return map(static_cast<long>(t), 20, 200, 20, CalTemp[CurrentTip][0]);

  if (t < 280)
    return map(static_cast<long>(t), 200, 280, CalTemp[CurrentTip][0], CalTemp[CurrentTip][1]);

  return map(static_cast<long>(t), 280, 360, CalTemp[CurrentTip][1], CalTemp[CurrentTip][2]);
}

*/