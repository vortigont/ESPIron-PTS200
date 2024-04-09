#pragma once
#include "config.h"
#include "FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include <ESP32AnalogRead.h>
#include "FastPID.h"

#define HEATER_MEASURE_RATE       10                    // Tip temperature measuring rate in working mode, Hz

// Define PID tuning parameters
constexpr float consKp = 5, consKi = 1, consKd = 6;

/**
 * @brief Class that manages Iron tip heating
 * 
 */
class TipHeater {

  enum class HeaterState_t {
    shutoff = 0,    // both heater control, PWM and temperature measurments are disabled
    inactive,       // heater is disabled, but tip temperature control is active
    active,         // heater is active, temperature control is engaged
    notip,          // failure to detect tip temperature or tip is missing
    fault           // temperature control was unable to stabilyze temperature, overhating, FET failure, etc...
  };

  struct Temperatures
  {
    // target Tip temperature the heater will try to match
    int32_t target;
    // averaged measured tip temperature
    float avg;
    // averaged temperature with applied calibration mapping
    int32_t calibrated;
  };

  // PWM related configuration
  struct CfgPWM {
    int gpio;
    ledc_channel_t channel;
    bool invert;
    uint32_t duty;
  };

  HeaterState_t _state{HeaterState_t::inactive};

  // temperatures
  Temperatures _t{};

  CfgPWM _pwm;

  TaskHandle_t    _task_hndlr = nullptr;

  // event handlers
  esp_event_handler_instance_t _evt_cmd_handler = nullptr;
  esp_event_handler_instance_t _evt_ntf_handler = nullptr;


  ESP32AnalogRead adc_sensor;

  // Specify variable pointers and initial PID tuning parameters
  // 指定变量指针和初始PID调优参数
  FastPID _pid = FastPID(consKp, consKi, consKd, HEATER_MEASURE_RATE);

  // static wrapper for _runner Task to call handling class member
  static inline void _runner(void* pvParams){ ((TipHeater*)pvParams)->_heaterControl(); }

  // events processing
  void _evt_picker(esp_event_base_t base, int32_t id, void* data);

  /**
   * @brief RTOS task runner that monitor tip temperature and controls heater PWM
   * 
   * @return * void 
   */
  void _heaterControl();

  /**
   * @brief starts RTOS PWM controlling task
   * it also includes tip temperature measuring
   */
  void _start_runner();

  /**
   * @brief stops RTOS PWM controlling task
   * it also stops tip temperature measuing
   */
  void _stop_runner();

  // 
  void _measureTipTemp();

  float _denoiseADC();

public:
  TipHeater(int gpio, ledc_channel_t channel = LEDC_CHANNEL_0, bool invert = false) : _pwm({gpio, channel, invert, 0}) {};
  ~TipHeater();

  /**
   * @brief initialize heater PWM engine
   * 
   */
  void init();

  /**
   * @brief enable heater
   * 
   */
  void enable();

  /**
   * @brief disable heater
   * 
   */
  void disable();

  /**
   * @brief Set target Tempearture in Celsius
   * 
   * @param t 
   */
  void setTargetTemp(int32_t t){ _t.target = t; };

  /**
   * @brief Get the Target heater temperature
   * 
   * @return int32_t 
   */
  int32_t getTargetTemp() const { return _t.target; }

  /**
   * @brief Get current tip temperature
   * will return averaged and calibrated temp value
   * @return int32_t 
   */
  int32_t getCurrentTemp() const { return _t.calibrated; }

};
