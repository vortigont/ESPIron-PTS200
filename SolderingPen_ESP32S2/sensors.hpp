#pragma once
#include <array>
#include "config.h"
// https://github.com/sparkfun/SparkFun_LIS2DH12_Arduino_Library
#include "SparkFun_LIS2DH12.h"
#include "ts.h"

#define ACCEL_SAMPLES 32

class GyroSensor {
  // G sensor axis metrics
  struct Gaxis {
    uint16_t x, y, z;
  };

  bool _motion{false};
  int _motionThreshold{WAKEUP_THRESHOLD};
  std::array<Gaxis, ACCEL_SAMPLES> samples;
  size_t accelIndex{0};
  //uint16_t accels[32][3];

  SPARKFUN_LIS2DH12 accel;
  Task _tPoller;

  void _poll();

  /**
   * @brief clear sampling array
   * 
   */
  void _clear();

public:
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
