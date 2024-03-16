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

struct Temperatures
{
  // target Tip temperature
  int32_t target;
  // tip temperature with applied calibration mapping
  int32_t calibrated;
  // averaged measured temperature
  float avg;

};

  bool enabled = false;

  // temperatures
  Temperatures _t{};

  // PWM
  int _gpio;
  ledc_channel_t _ledc_channel;
  bool _invert;

  // working PWM duty
  uint32_t _pwm_duty{0};


  TaskHandle_t    _task_hndlr = nullptr;

  ESP32AnalogRead adc_sensor;

  // Specify variable pointers and initial PID tuning parameters
  // 指定变量指针和初始PID调优参数
  FastPID _pid = FastPID(consKp, consKi, consKd, HEATER_MEASURE_RATE);

  // static wrapper for _runner Task to call handling class member
  static inline void _runner(void* pvParams){ ((TipHeater*)pvParams)->_runnerHndlr(); }

  // member function that runs RTOS task
  void _runnerHndlr();

  // starts RTOS task
  void _start_runner();

  // 
  void _measureTipTemp();

  float _denoiseADC();

public:
  TipHeater(int gpio, ledc_channel_t ledc_channel = LEDC_CHANNEL_0, bool invert = false) : _gpio(gpio), _ledc_channel(ledc_channel), _invert(invert) {};
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
