#pragma once
#include <array>
#include "config.h"
// https://github.com/sparkfun/SparkFun_LIS2DH12_Arduino_Library
#include "SparkFun_LIS2DH12.h"
#include "ts.h"
#include "evtloop.hpp"
#include "freertos/timers.h"

#define GYRO_ACCEL_SAMPLES 32

class GyroSensor {
  // G sensor axis metrics
  struct Gaxis {
    uint16_t x, y, z;
  };

  bool _motion{false};
  int _motionThreshold{WAKEUP_THRESHOLD};
  std::array<Gaxis, GYRO_ACCEL_SAMPLES> samples;
  size_t accelIndex{0};
  //uint16_t accels[32][3];

  SPARKFUN_LIS2DH12 accel;
  // temperature polling timer
  TimerHandle_t _tmr_temp = nullptr;
  // accell sensor polling timer
  TimerHandle_t _tmr_accel = nullptr;

  /**
   * @brief clear sampling array
   * 
   */
  void _clear();

  /**
   * @brief poll temperature sensor inside accell chip and publish to event message bus
   * 
   */
  void _temperature_poll();

  /**
   * @brief poll gyro sensor and detect motion
   * 
   */
  void _accel_poll();


public:
  // d-tor
  ~GyroSensor();

  /**
   * @brief initialize sensor
   * 
   */
  void begin();

  /**
   * @brief enable motion sensor detector
   * 
   */
  void enable();

  /**
   * @brief disable motion sensor detector
   * 
   */
  void disable();

  /**
   * @brief check if motion was detected
   * state will reset on request
   * this is hackish method unless I implement event generation
   * 
   * @return true 
   * @return false 
   */
  bool motionDetect();

  /**
   * @brief Set Motion Threshold value
   * the lower the value, the more sensitive sensor is
   * default is 10
   * 
   * @param m 
   */
  void setMotionThreshold(int m){ _motionThreshold = m; };

  /**
   * @brief Get accell chip temperature
   * 
   * @return float temp in Celsius
   */
  float getAccellTemp();
};

