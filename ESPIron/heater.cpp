#include <array>
#include "const.h"
#include "evtloop.hpp"
#include "heater.hpp"
#include "log.h"

#define HEATER_TASK_PRIO          tskIDLE_PRIORITY+1    // task priority
#ifdef PTS200_DEBUG_LEVEL
#define HEATER_TASK_STACK         2048                  // sprintf could take lot's of stack mem for debug messages
#else
#define HEATER_TASK_STACK         1024
#endif
#define HEATER_TASK_NAME          "HEATER"

#define HEATER_LEDC_SPEEDMODE     LEDC_LOW_SPEED_MODE             // S2 supports only low speed mode
#define HEATER_LEDC_DUTY_RES      HEATER_RES
#define HEATER_LEDC_TIMER         LEDC_TIMER_0
#define HEATER_LEDC_FREQUENCY     HEATER_FREQ
#define HEATER_LEDC_RAMPUP_TIME   3000                  // time duration to fade in-for PWM ramping, ms

#define HEATER_OPAMP_STABILIZE_MS 8                     // how long to wait after disabling PWM to let OpAmp stabilize

#define HEATER_ADC_SAMPLES        8                     // number of ADC reads to averate tip tempearture

#define PID_ENGAGE_DIFF_LOW       30                    // lower temperature difference when PID algo should be engaged
#define PID_ENGAGE_DIFF_HIGH      10                    // higher temperature difference when PID algo should be engaged


// normal interval between temp measurments
constexpr TickType_t measure_delay_ticks = pdMS_TO_TICKS(1000 / HEATER_MEASURE_RATE);
// long interval between temp measurments
constexpr TickType_t long_measure_delay_ticks = pdMS_TO_TICKS(500);
// idle interval between temp measurments
constexpr TickType_t idle_delay_ticks = pdMS_TO_TICKS(1000);

// a simple constrain function
template<typename T>
T clamp(T value, T min, T max){
  return (value < min)? min : (value > max)? max : value;
}

TipHeater::~TipHeater(){
  // unsubscribe from event bus
  if (_evt_cmd_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_SET_EVT, ESP_EVENT_ANY_ID, _evt_cmd_handler);
    _evt_cmd_handler = nullptr;
  }
/*
  if (_evt_ntf_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_NOTIFY, ESP_EVENT_ANY_ID, _evt_ntf_handler);
    _evt_ntf_handler = nullptr;
  }
*/
  _stop_runner();
}

void TipHeater::init(){
  // set PID output range
  _pid.setOutputRange(0, 1<<HEATER_RES);

  // event bus subscriptions
  if (!_evt_cmd_handler){
    esp_event_handler_instance_register_with(
      evt::get_hndlr(),
      IRON_HEATER,
      ESP_EVENT_ANY_ID,
      [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<TipHeater*>(self)->_evt_picker(base, id, data); },
      this,
      &_evt_cmd_handler
    );
  }
/*
  if (!_evt_ntf_handler){
    esp_event_handler_instance_register_with(
      evt::get_hndlr(),
      IRON_NOTIFY,
      ESP_EVENT_ANY_ID,
      [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<TipHeater*>(self)->_evt_picker(base, id, data); },
      this,
      &_evt_ntf_handler
    );
  }
*/

  // init HW fader
  ledc_fade_func_install(0);
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

    case evt::iron_t::heaterEnable : {
      // enable heater
      enable();
      return;
    }

    case evt::iron_t::heaterDisable : {
      // disable heater in standby mode
      disable();
      return;
    }

    case evt::iron_t::heaterRampUp : {
      // disable heater in standby mode
      rampUp();
      return;
    }

/*
    // obsolete, use deep-sleep in Suspend
    case evt::iron_t::stateSuspend : {
      // disable worker task in standby mode
      _stop_runner();
      return;
    }
*/
  }
}


void TipHeater::_start_runner(){
  // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {
      .speed_mode       = HEATER_LEDC_SPEEDMODE,
      .duty_resolution  = HEATER_LEDC_DUTY_RES,        
      .timer_num        = HEATER_LEDC_TIMER,
      .freq_hz          = HEATER_LEDC_FREQUENCY,
      .clk_cfg          = LEDC_AUTO_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {
      .gpio_num       = _pwm.gpio,
      .speed_mode     = HEATER_LEDC_SPEEDMODE,
      .channel        = _pwm.channel,
      .intr_type      = LEDC_INTR_DISABLE,
      .timer_sel      = HEATER_LEDC_TIMER,
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

  // fader iterrupt handler, it will resume heaterControl task on fade end event
  ledc_cbs_t fade_callback = { 
    .fade_cb = TipHeater::_cb_ledc_fade_end_event
  };
  // register fader interrupt handler
  ledc_cb_register(HEATER_LEDC_SPEEDMODE, _pwm.channel, &fade_callback, this);
}

void TipHeater::_stop_runner(){
  _state = HeaterState_t::shutoff;
  ledc_stop(HEATER_LEDC_SPEEDMODE, _pwm.channel, _pwm.invert);
  if(_task_hndlr)
    vTaskDelete(_task_hndlr);
  _task_hndlr = nullptr;

  // unregister cb interrupt handler
  ledc_fade_func_uninstall();
}

void TipHeater::_heaterControl(){
  TickType_t xLastWakeTime = xTaskGetTickCount();
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
        // use while to avoid concurency issues with hw fader interrupt
        while (ledc_get_duty(HEATER_LEDC_SPEEDMODE, _pwm.channel)){
          // reset run time (need to avoid extra consecutive runs with xTaskDelayUntil if thread was suspended from the outside)
          xLastWakeTime = xTaskGetTickCount();
          // shut off heater in order to measure temperature 关闭加热器以测量温度
          ledc_set_duty(HEATER_LEDC_SPEEDMODE, _pwm.channel, 0);
          ledc_update_duty(HEATER_LEDC_SPEEDMODE, _pwm.channel);
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
      ledc_set_duty(HEATER_LEDC_SPEEDMODE, _pwm.channel, 0);
      ledc_update_duty(HEATER_LEDC_SPEEDMODE, _pwm.channel);
      _state = HeaterState_t::notip;
      LOGW(T_HEAT, printf, "Iron Tip ejected, T:%d\n", static_cast<int32_t>(t));
      EVT_POST(SENSOR_DATA, e2int(evt::iron_t::tipEject));
      continue;
    }

    // check if we've get the Tip back
    if (_state == HeaterState_t::notip && t < TEMP_NOTIP){
      _state = HeaterState_t::active;
      EVT_POST(SENSOR_DATA, e2int(evt::iron_t::tipInsert));
      LOGW(T_HEAT, println, "Iron Tip inserted");
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
    ADC_LOGV(T_ADC, printf, "avg T: %5.1f, cal T: %d, tgt T:%d\n", _t.avg, _t.calibrated, _t.target);
    EVT_POST_DATA(SENSOR_DATA, e2int(evt::iron_t::tiptemp), &_t.calibrated, sizeof(_t.calibrated));

    // if PID algo should be engaged
    if (_t.calibrated > (_t.target - PID_ENGAGE_DIFF_LOW) && _t.calibrated < (_t.target + PID_ENGAGE_DIFF_HIGH)){
      _pwm.duty = _pid.step(_t.target, _t.calibrated);
      delay_time = measure_delay_ticks;
    } else {
      // heater must be either turned full on or off
      _pwm.duty = _t.calibrated < _t.target ? 1<<HEATER_RES : 0;
      _pid.clear();   // reset pid algo
      // give heater more time to gain/loose temperature
      delay_time = long_measure_delay_ticks;
    }

    ledc_set_duty(HEATER_LEDC_SPEEDMODE, _pwm.channel,  _pwm.duty);
    ledc_update_duty(HEATER_LEDC_SPEEDMODE, _pwm.channel);
    PWM_LOGV(T_HEAT, printf, "Duty:%u\n",  _pwm.duty);
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

  LOGI(T_HEAT, printf, "Enable, target T:%d\n", _t.target);
};

void TipHeater::disable(){
  switch (_state){
    case HeaterState_t::active :
      ledc_set_duty(HEATER_LEDC_SPEEDMODE, _pwm.channel, 0);
      ledc_update_duty(HEATER_LEDC_SPEEDMODE, _pwm.channel);
      _state = HeaterState_t::inactive;
      LOGI(T_PWM, println, "Disable");
      break;
    // in all other cases this call could be ignored
  }
}

void TipHeater::rampUp(){
  // can only ramp-up from inactive state
  if (_state != HeaterState_t::inactive) return;

  // first suspend heaterControl task for the duration of fade-in, 'cause no changes to PWM duty could be made for that period,
  // fader interrupt will resume it
  vTaskSuspend( _task_hndlr );
  // initiate HW fader
  ledc_set_fade_time_and_start(HEATER_LEDC_SPEEDMODE, _pwm.channel, 1<<HEATER_RES, HEATER_LEDC_RAMPUP_TIME, LEDC_FADE_NO_WAIT);
  // from now on consider heater in active state
  _state = HeaterState_t::active;
}

bool TipHeater::_cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *arg){
  // do not care what was the event, I need to unblock heater control anyway
  // if (param->event == LEDC_FADE_END_EVT)

  BaseType_t task_awoken{0};
  // notify that rampUp has been complete
  EVT_POST_ISR(IRON_NOTIFY, e2int(evt::iron_t::statePWRRampCmplt), &task_awoken);
  task_awoken |= xTaskResumeFromISR(static_cast<TipHeater*>(arg)->_task_hndlr);
  return task_awoken;
}


// 对32个ADC读数进行平均以降噪
//  VP+_Ru = 100k, Rd_GND = 1K
float TipHeater::_denoiseADC(){
  std::array<uint32_t, HEATER_ADC_SAMPLES> samples;

  for (auto &i : samples){
    i = analogReadMilliVolts(TIP_ADC_SENSOR_PIN);
    // value = constrain(0.4432 * raw_adc + 29.665, 20, 1000);
  }

  // sort resultArray with low time complexity
  // 用低时间复杂度对resultArray进行排序
  for (size_t i = 0; i < samples.size(); ++i){
    for (size_t j = i + 1; j < samples.size(); ++j){
      if (samples[i] > samples[j]) {
        std::swap(samples[i], samples[j]);
      }
    }
  }

  uint32_t result{0};
  // get the average of the middle 4 readings 获取中间20个读数的平均值
  for (uint8_t i = 2; i < 6; i++) {
    result += samples[i];
  }
  result /= 4;

  // convert mV to Celsius
  float t = clamp(0.5378 * result + 6.3959, 20.0, 1000.0);
  ADC_LOGV(T_ADC, printf, "avg mV:%u / Temp C:%6.1f\n", result, t);
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