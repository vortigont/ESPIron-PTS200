#include "sensors.hpp"
#include "const.h"
#include "log.h"

#define LIS
#define MOTION_FACTOR 25000
#define MOTION_POLL_PERIOD    50  // ms

GyroSensor::~GyroSensor(){
  ts.deleteTask(_tPoller);
};

void GyroSensor::begin(){
  if (!accel.begin()) {
    LOGE(T_GYRO, println, "Accelerometer not detected.");
  }

  _tPoller.set(MOTION_POLL_PERIOD, TASK_FOREVER, [this](){ _poll(); });
  ts.addTask(_tPoller);
  _tPoller.enable();

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

void GyroSensor::_poll(){
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

  if (accelIndex != ACCEL_SAMPLES)
    return;

  accelIndex = 0;
  // cal variance
  uint32_t avg[3] = {0, 0, 0};
  for (auto &i : samples){
    avg[0] += i.x;
    avg[1] += i.y;
    avg[2] += i.z;
  }

  avg[0] /= ACCEL_SAMPLES;
  avg[1] /= ACCEL_SAMPLES;
  avg[2] /= ACCEL_SAMPLES;

  uint32_t var[3] = {0, 0, 0};
  for (auto &i : samples){
    var[0] += (i.x - avg[0]) * (i.x - avg[0]);
    var[1] += (i.y - avg[1]) * (i.y - avg[1]);
    var[2] += (i.z - avg[2]) * (i.z - avg[2]);
  }

  var[0] /= ACCEL_SAMPLES;
  var[1] /= ACCEL_SAMPLES;
  var[2] /= ACCEL_SAMPLES;

  // debug output
  // Serial.print("variance: ");
  // Serial.print(var[0]);
  // Serial.print(" ");
  // Serial.print(var[1]);
  // Serial.print(" ");
  // Serial.println(var[2]);

  int varThreshold = _motionThreshold * MOTION_FACTOR;

  if (var[0] > varThreshold || var[1] > varThreshold || var[2] > varThreshold) {
    _motion = true;
    _clear();
    LOGD(T_GYRO, println, "motion detected!");
    LOGD(T_GYRO, printf, "Th:%d, x:%u, y:%u, z:%u\n", varThreshold, var[0], var[1], var[2]);
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
  _tPoller.enable();
}

void GyroSensor::disable(){
  _tPoller.disable();
}

void GyroSensor::_clear(){
  Gaxis axis;
  axis.x = accel.getRawX() + 32768;
  axis.y = accel.getRawY() + 32768;
  axis.z = accel.getRawZ() + 32768;

  samples.fill(axis);
}
