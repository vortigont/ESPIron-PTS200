#include "sensors.hpp"
#include "const.h"
#include "log.h"

#define LIS
#define ACCEL_MOTION_FACTOR 25000
#define ACCEL_MOTION_POLL_PERIOD        50      // ms
#define ACCEL_TEMPERATURE_POLL_PERIOD   1000    // ms


GyroSensor::~GyroSensor(){
  xTimerDelete( _tmr_temp, portMAX_DELAY );
  xTimerDelete( _tmr_accel, portMAX_DELAY );
  _tmr_temp = nullptr;
  _tmr_accel = nullptr;
};

void GyroSensor::begin(){
  if (!accel.begin()) {
    LOGE(T_GYRO, println, "Accelerometer not detected.");
  }

//  _tPoller.set(ACCEL_MOTION_POLL_PERIOD, TASK_FOREVER, [this](){ _poll(); });
//  ts.addTask(_tPoller);
//  _tPoller.enable();

  // start temperature polling
  if (!_tmr_temp){
    _tmr_temp = xTimerCreate("gyroT",
                              pdMS_TO_TICKS(ACCEL_TEMPERATURE_POLL_PERIOD),
                              pdTRUE,
                              static_cast<void*>(this),
                              [](TimerHandle_t h) { static_cast<GyroSensor*>(pvTimerGetTimerID(h))->_temperature_poll(); }
                            );
    if (_tmr_temp)
      xTimerStart( _tmr_temp, pdMS_TO_TICKS(10) );
  }

  // start accl polling
  if (!_tmr_accel){
    _tmr_accel = xTimerCreate("gyroA",
                              pdMS_TO_TICKS(ACCEL_MOTION_POLL_PERIOD),
                              pdTRUE,
                              static_cast<void*>(this),
                              [](TimerHandle_t h) { static_cast<GyroSensor*>(pvTimerGetTimerID(h))->_accel_poll(); }
                            );
    // keep it disabled on begin
    //xTimerStart( _tmr_accel, pdMS_TO_TICKS(10) );
  }

  // lis2dh12_block_data_update_set(&(accel.dev_ctx), PROPERTY_DISABLE);
  // accel.setScale(LIS2DH12_2g);
  // accel.setMode(LIS2DH12_HR_12bit);
  // accel.setDataRate(LIS2DH12_ODR_400Hz);
  // lis2dh12_fifo_mode_set(&(accel.dev_ctx), LIS2DH12_BYPASS_MODE);
}

bool GyroSensor::motionDetect(){
  bool out = _motion;
  _motion = false;
  return out;
}

void GyroSensor::_accel_poll(){
  /*#if defined(MPU)
    mpu6050.update();
    if (abs(mpu6050.getGyroX() - gx) > WAKEUP_THRESHOLD ||
    abs(mpu6050.getGyroY() - gy) > WAKEUP_THRESHOLD || abs(mpu6050.getGyroZ() -
    gz) > WAKEUP_THRESHOLD)
    {
      gx = mpu6050.getGyroX();
      gy = mpu6050.getGyroY();
      gz = mpu6050.getGyroZ();
      handleMoved = true;
      Serial.println("进入工作状态!");
    }*/
  // #if defined(LIS)

  // if (abs(accel.getX() - gx) > WAKEUPthreshold ||
  //     abs(accel.getY() - gy) > WAKEUPthreshold ||
  //     abs(accel.getZ() - gz) > WAKEUPthreshold) {
  //   gx = accel.getX();
  //   gy = accel.getY();
  //   gz = accel.getZ();
  //   handleMoved = true;
  //   //    Serial.println("进入工作状态!");
  // }

  // accel.getRawX() return int16_t

  if (!accel.available())
    return;

  //samples.at(0).x
  samples.at(accelIndex).x = accel.getRawX() + 32768;
  samples.at(accelIndex).y = accel.getRawY() + 32768;
  samples.at(accelIndex).z = accel.getRawZ() + 32768;
  accelIndex++;

  // debug output
  // Serial.print("X: ");
  // Serial.print(accels[accelIndex][0]);
  // Serial.print(" Y: ");
  // Serial.print(accels[accelIndex][1]);
  // Serial.print(" Z: ");
  // Serial.println(accels[accelIndex][2]);

  if (accelIndex != GYRO_ACCEL_SAMPLES)
    return;

  accelIndex = 0;
  // cal variance
  uint32_t avg[3] = {0, 0, 0};
  for (auto &i : samples){
    avg[0] += i.x;
    avg[1] += i.y;
    avg[2] += i.z;
  }

  avg[0] /= GYRO_ACCEL_SAMPLES;
  avg[1] /= GYRO_ACCEL_SAMPLES;
  avg[2] /= GYRO_ACCEL_SAMPLES;

  uint32_t var[3] = {0, 0, 0};
  for (auto &i : samples){
    var[0] += (i.x - avg[0]) * (i.x - avg[0]);
    var[1] += (i.y - avg[1]) * (i.y - avg[1]);
    var[2] += (i.z - avg[2]) * (i.z - avg[2]);
  }

  var[0] /= GYRO_ACCEL_SAMPLES;
  var[1] /= GYRO_ACCEL_SAMPLES;
  var[2] /= GYRO_ACCEL_SAMPLES;

  // debug output
  // Serial.print("variance: ");
  // Serial.print(var[0]);
  // Serial.print(" ");
  // Serial.print(var[1]);
  // Serial.print(" ");
  // Serial.println(var[2]);

  int varThreshold = _motionThreshold * ACCEL_MOTION_FACTOR;

  if (var[0] > varThreshold || var[1] > varThreshold || var[2] > varThreshold) {
    _motion = true;
    _clear();
    LOGD(T_GYRO, println, "motion detected!");
    LOGD(T_GYRO, printf, "Th:%d, x:%u, y:%u, z:%u\n", varThreshold, var[0], var[1], var[2]);
    // post event with motion detect
    EVT_POST(SENSOR_DATA, e2int(evt::iron_t::motion));
  }
}

// get LIS/MPU temperature 获取LIS/MPU的温度
float GyroSensor::getAccellTemp() {
#if defined(MPU)
  mpu6050.update();
  return mpu6050.getTemp();
#elif defined(LIS)
  return accel.getTemperature();
#endif
}

void GyroSensor::enable(){
  _clear();
  if (_tmr_accel)
    xTimerStart( _tmr_accel, pdMS_TO_TICKS(10) );
}

void GyroSensor::disable(){
  if (_tmr_accel)
    xTimerStop( _tmr_accel, pdMS_TO_TICKS(10) );
}

void GyroSensor::_clear(){
  Gaxis axis;
  axis.x = accel.getRawX() + 32768;
  axis.y = accel.getRawY() + 32768;
  axis.z = accel.getRawZ() + 32768;

  samples.fill(axis);
}

void GyroSensor::_temperature_poll(){
  float t =  getAccellTemp();
  EVT_POST_DATA(SENSOR_DATA, e2int(evt::iron_t::acceltemp), &t, sizeof(t));
}