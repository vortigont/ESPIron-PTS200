#include "sensors.hpp"
#include "nvs_handle.hpp"
#include "const.h"
#include "log.h"

#define LIS
#define ACCEL_MOTION_FACTOR             25000
#define ACCEL_MOTION_POLL_PERIOD        50      // ms
#define ACCEL_TEMPERATURE_POLL_PERIOD   3000    // ms

#define VIN_ADC_POLL_PERIOD             1500    // ms

GyroSensor::~GyroSensor(){
  // unsubscribe from event bus
  if (_evt_set_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_SET_EVT, e2int(evt::iron_t::sensorsReload), _evt_set_handler);
    _evt_set_handler = nullptr;
  }

  xTimerDelete( _tmr_temp, portMAX_DELAY );
  xTimerDelete( _tmr_accel, portMAX_DELAY );
  _tmr_temp = nullptr;
  _tmr_accel = nullptr;
};

void GyroSensor::init(){
  if (!accel.begin()) {
    LOGE(T_GYRO, println, "Accelerometer not detected.");
    return;
  }
  LOGI(T_Sensor, println, "Init Accelerometer sensor");

  // start temperature polling
  if (!_tmr_temp){
    _tmr_temp = xTimerCreate("gyroT",
                              pdMS_TO_TICKS(ACCEL_TEMPERATURE_POLL_PERIOD),
                              pdTRUE,
                              static_cast<void*>(this),
                              [](TimerHandle_t h) { static_cast<GyroSensor*>(pvTimerGetTimerID(h))->_temperature_poll(); }
                            );
    //if (_tmr_temp)
    //  xTimerStart( _tmr_temp, portMAX_DELAY );
  }

  // start accl polling
  if (!_tmr_accel){
    _tmr_accel = xTimerCreate("gyroA",
                              pdMS_TO_TICKS(ACCEL_MOTION_POLL_PERIOD),
                              pdTRUE,
                              static_cast<void*>(this),
                              [](TimerHandle_t h) { static_cast<GyroSensor*>(pvTimerGetTimerID(h))->_accel_poll(); }
                            );
    // TODO: realct on events from mode changes
    //if (_tmr_accel)
    //  xTimerStart( _tmr_accel, portMAX_DELAY );
  }

  // subscribe to event bus
  if (!_evt_set_handler){
    esp_event_handler_instance_register_with(
      evt::get_hndlr(),
      IRON_SET_EVT,
      e2int(evt::iron_t::sensorsReload),    // subscribe to 'sensorsReload command'
      [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<GyroSensor*>(self)->enable(); },    // trigger config and timers reload
      this,
      &_evt_set_handler
    );
  }

  // start sensor polling
  enable();
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

  uint32_t varThreshold = _motionThreshold * ACCEL_MOTION_FACTOR;

  if (var[0] > varThreshold || var[1] > varThreshold || var[2] > varThreshold) {
    _motion = true;
    _clear();
    LOGD(T_GYRO, println, "motion detected!");
    LOGV(T_GYRO, printf, "Th:%d, x:%u, y:%u, z:%u\n", varThreshold, var[0], var[1], var[2]);
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

  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> nvs = nvs::open_nvs_handle(T_Sensor, NVS_READONLY, &err);

  // load configured motion threshold
  if (err == ESP_OK) {
    _motionThreshold = WAKEUP_THRESHOLD;
    nvs->get_item(T_motionThr, _motionThreshold);
  }

  // start sensor polling
  if (_tmr_accel)
    xTimerStart( _tmr_accel, portMAX_DELAY );

  if (_tmr_temp)
    xTimerStart( _tmr_temp, portMAX_DELAY );
  LOGD(T_GYRO, printf, "accel sensor enabled, thr:%u\n", _motionThreshold);
}

void GyroSensor::disable(){
  if (_tmr_accel)
    xTimerStop( _tmr_accel, portMAX_DELAY );

  if (_tmr_temp)
    xTimerStop( _tmr_temp, portMAX_DELAY );
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



// *** VinSensor methods ***

void VinSensor::init(){
  LOGI(T_Sensor, println, "Init Voltage sensor");
  // input voltage pin ADC
  adc_vin.attach(VIN_PIN);

  // start voltage polling
  if (!_tmr_runner){
    _tmr_runner = xTimerCreate("vinADC",
                              pdMS_TO_TICKS(VIN_ADC_POLL_PERIOD),
                              pdTRUE,
                              static_cast<void*>(this),
                              [](TimerHandle_t h) { static_cast<VinSensor*>(pvTimerGetTimerID(h))->_runner(); }
                            );
    // start poller right away
    if (_tmr_runner && xTimerStart( _tmr_runner, portMAX_DELAY ) == pdPASS){
      LOGD(T_Sensor, println, "Enable Input voltage polling");
    } else {
      LOGE(T_Sensor, println, "Unable to start voltage polling");
    }
  }

/*
  // subscribe to event bus
  if (!_evt_set_handler){
    esp_event_handler_instance_register_with(
      evt::get_hndlr(),
      IRON_SET_EVT,
      e2int(evt::iron_t::sensorsReload),    // subscribe to 'sensorsReload command'
      [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<VinSensor*>(self)->enable(); },    // trigger config and timers reload
      this,
      &_evt_set_handler
    );
  }
*/
}

VinSensor::~VinSensor(){
  xTimerDelete( _tmr_runner, portMAX_DELAY );
  _tmr_runner = nullptr;
}

// get supply voltage in mV 得到以mV为单位的电源电压
void VinSensor::_runner(){
  uint32_t voltage = 0;

  for (uint8_t i = 0; i < 4; i++) {  // get 32 readings 得到32个读数
    voltage += adc_vin.readMiliVolts();
  }
  voltage = voltage / 4 * 31.3f;

  // log and publish Vin value
  ADC_LOGV(T_ADC, printf, "Vin: %u mV\n", voltage);
  EVT_POST_DATA(SENSOR_DATA, e2int(evt::iron_t::vin), &voltage, sizeof(voltage));


  //  some calibration calc
  //  // VIN_Ru = 100k, Rd_GND = 3.3K
  //  if (value < 500)
  //  {
  //    voltage = value * 1390 * 31.3 / 4095 * 1.35;
  //  }
  //  else if (500 <= value && value < 1000)
  //  {
  //    voltage = value * 1390 * 31.3 / 4095 * 1.135;
  //  }
  //  else if (1000 <= value && value < 1500)
  //  {
  //    voltage = value * 1390 * 31.3 / 4095 * 1.071;
  //  }
  //  else if (1500 <= value && value < 2000)
  //  {
  //    voltage = value * 1390 * 31.3 / 4095;
  //  }
  //  else if (2000 <= value && value < 3000)
  //  {
  //    voltage = value * 1390 * 31.3 / 4095;
  //  }
  //  else
  //    voltage = value * 1390 * 31.3 / 4095;
}