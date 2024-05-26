/*
    This file is a part of ESPIron-PTS200 project
    https://github.com/vortigont/ESPIron-PTS200

    Copyright © 2024 Emil Muratov (vortigont)

    ESPIron-PTS200 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/
#pragma once
#include "evtloop.hpp"
#include "common.hpp"
#include "const.h"
#include "nvs.hpp"
#include "freertos/timers.h"
#include <QC3Control.h>

// forward declaration
class QC3ControlWA;


/**
 * @brief An object that represents Iron states and configuration
 * 
 */
class IronController {

  struct TickStamps {
    // last time motion was detected
    TickType_t motion;
    TickType_t boost;
    TickType_t idle;
  };

  IronTimeouts _timeout;
  Temperatures _temp;
  TickStamps _xTicks;

  // current iron mode
  ironState_t _state{ironState_t::idle};

  // working voltage, set default to 20v to let PD trigger select if no value set in NVS
  uint32_t _voltage{20};

  // Use PWM power ramping
  bool _pwm_ramp{false};

  // Mode Switcher timer
  TimerHandle_t _tmr_mode = nullptr;

  // sensor events handler
  esp_event_handler_instance_t _evt_sensor_handler = nullptr;

  // command events handler
  esp_event_handler_instance_t _evt_cmd_handler = nullptr;

  // request commands events handler
  esp_event_handler_instance_t _evt_req_handler = nullptr;

  // QC Trigger
  std::unique_ptr<QC3ControlWA> _qc;

  /**
   * @brief timer callback that watches mode switching
   * it maintains timeouts for sleep, off, boost modes, etc...
   * 
   */
  void _mode_switcher();

  // event bus messages dispatcher
  static void _event_hndlr(void* handler, esp_event_base_t base, int32_t id, void* event_data);

  // sensors events executor
  void _evt_sensors(esp_event_base_t base, int32_t id, void* data);

  // commands events executor
  void _evt_commands(esp_event_base_t base, int32_t id, void* data);

  // req commands events executor
  void _evt_reqs(esp_event_base_t base, int32_t id, void* data);

public:
  //IronController() : _qcc(QC_DP_PIN, QC_DM_PIN) {}
  ~IronController();

  /**
   * @brief Initialize Iron controller
   * 
   */
  void init();


  void setTimeOutStandby(unsigned v){ _timeout.standby = v; _saveTimeouts(); }
  void setTimeOutSuspend(unsigned v){ _timeout.suspend = v; _saveTimeouts(); }
  void setTimeOutBoost(unsigned v){ _timeout.boost = v; _saveTimeouts(); }

  void setTempWorking(unsigned v){ _temp.working = v; _saveTemp(); }
  void setTempStandby(unsigned v){ _temp.standby = v; _saveTemp(); }
  void setTempBoost(unsigned v){ _temp.boost = v; _saveTemp(); }

  // get internal temperatures configuration
  const Temperatures& getTemperatures() const { return _temp; }

private:
  /**
   * @brief save current timeout values to NVS
   * 
   */
  void _saveTimeouts(){ nvs_blob_write(T_IRON, T_timeouts, static_cast<void*>(&_timeout), sizeof(IronTimeouts)); };

  /**
   * @brief save current temperature values to NVS
   * 
   */
  void _saveTemp(){ nvs_blob_write(T_IRON, T_temperatures, static_cast<void*>(&_timeout), sizeof(IronTimeouts)); };

  /**
   * @brief PD Trigger control
   * 
   * @param voltage 
   */
  void _pd_trigger(uint32_t voltage);

  /**
   * @brief init trigger gpios and resotre working voltage from NVS
   * 
   */
  void _pd_trigger_init();
};

/**
 * @brief a wrapper class for QCControl
 * it makes long blocking operations with QC asynchronous
 * and uses different call methods for QC2/QC3
 * 
 */
class QC3ControlWA : public QC3Control {

  // Delay Switcher timer
  TimerHandle_t _tmr_mode;
  uint32_t qc_mode, qcv, pending_qcv{0};

  void _start();

public:
  QC3ControlWA(uint32_t mode, uint32_t voltage);
  virtual ~QC3ControlWA();

  void setQCV(uint32_t V);
};

