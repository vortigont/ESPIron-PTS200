/*
    This file is a part of ESPIron-PTS200 project
    https://github.com/vortigont/ESPIron-PTS200

    Copyright © 2024 Emil Muratov (vortigont)

    ESPIron-PTS200 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "ironcontroller.hpp"
#include "main.h"
#include "log.h"

#define MODE_TIMER_PERIOD   1000

using namespace evt;

IronController::~IronController(){
  // unsubscribe from event bus
  if (_evt_sensor_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), SENSOR_DATA, ESP_EVENT_ANY_ID, _evt_sensor_handler);
    _evt_sensor_handler = nullptr;
  }

  if (_evt_cmd_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_SET_EVT, ESP_EVENT_ANY_ID, _evt_cmd_handler);
    _evt_cmd_handler = nullptr;
  }

  if (_evt_req_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_GET_EVT, ESP_EVENT_ANY_ID, _evt_req_handler);
    _evt_req_handler = nullptr;
  }
}

void IronController::init(){

  // load timeout values from NVS
  nvs_blob_read(T_IRON, T_timeouts, static_cast<void*>(&_timeout), sizeof(Timeouts));

  // load temperature values from NVS
  nvs_blob_read(T_IRON, T_temperatures, static_cast<void*>(&_temp), sizeof(Temperatures));

  // start mode switcher timer
  if (!_tmr_mode){
    _tmr_mode = xTimerCreate("modeT",
                              pdMS_TO_TICKS(MODE_TIMER_PERIOD),
                              pdTRUE,
                              static_cast<void*>(this),
                              [](TimerHandle_t h) { static_cast<IronController*>(pvTimerGetTimerID(h))->_mode_switcher(); }
                            );
    if (_tmr_mode)
      xTimerStart( _tmr_mode, pdMS_TO_TICKS(10) );
  }

  // event bus subscriptions
  if (!_evt_sensor_handler){
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(evt::get_hndlr(), SENSOR_DATA, ESP_EVENT_ANY_ID, IronController::_event_hndlr, this, &_evt_sensor_handler));
  }

  if (!_evt_cmd_handler){
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(evt::get_hndlr(), IRON_SET_EVT, ESP_EVENT_ANY_ID, IronController::_event_hndlr, this, &_evt_cmd_handler));
  }

  if (!_evt_req_handler){
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(evt::get_hndlr(), IRON_GET_EVT, ESP_EVENT_ANY_ID, IronController::_event_hndlr, this, &_evt_req_handler));
  }
}

void IronController::_mode_switcher(){

  switch (_state){
    // In working state
    case ironState_t::working : {
      if (pdTICKS_TO_MS(xTaskGetTickCount()) - pdTICKS_TO_MS(_xTicks.motion) > _timeout.standby){
        _state = ironState_t::standby;
        LOGI(T_CTRL, printf, "Engage standby mode due to sleep timeout of %u ms. Temp:%u\n", _timeout.standby, _temp.standby);
        // switch heater temperature to standby value
        EVT_POST_DATA(IRON_SET_EVT, e2int(iron_t::heaterTargetT), &_temp.standby, sizeof(_temp.standby));
        // notify other componets that we are switching to 'standby' mode
        EVT_POST(IRON_NOTIFY, e2int(iron_t::stateStandby));
      }
      return;
    }

    case ironState_t::standby : {
      if (pdTICKS_TO_MS(xTaskGetTickCount()) - pdTICKS_TO_MS(_xTicks.motion) > _timeout.idle){
        _state = ironState_t::idle;
        LOGI(T_CTRL, printf, "Engage idle mode due to idle timeout of %u ms\n", _timeout.idle);
        // notify other componets that we are switching to 'idle' mode
        EVT_POST(IRON_NOTIFY, e2int(iron_t::stateIdle));
      } else if (pdTICKS_TO_MS(xTaskGetTickCount()) - pdTICKS_TO_MS(_xTicks.motion) < _timeout.standby){
        // standby cancelled
        _state = ironState_t::working;
        LOGI(T_CTRL, println, "cancel Standby mode");
        // notify other componets that we are switching to 'work' mode
        EVT_POST_DATA(IRON_SET_EVT, e2int(iron_t::heaterTargetT), &_temp.working, sizeof(_temp.working));
        EVT_POST(IRON_NOTIFY, e2int(iron_t::stateWorking));
      }
      return;
    }

    case ironState_t::suspend : {
      if (pdTICKS_TO_MS(xTaskGetTickCount()) - pdTICKS_TO_MS(_xTicks.motion) > _timeout.suspend){
        _state = ironState_t::suspend;
        LOGI(T_CTRL, printf, "Engage suspend mode due to suspend timeout of %u ms\n", _timeout.suspend);
        // notify other componets that we are switching to 'idle' mode
        EVT_POST(IRON_NOTIFY, e2int(iron_t::stateSuspend));
      }
      return;
    }

    case ironState_t::boost : {
      if (pdTICKS_TO_MS(xTaskGetTickCount()) - pdTICKS_TO_MS(_xTicks.boost) > _timeout.boost){
        _state = ironState_t::working;
        // notify other componets that we are switching to 'working' mode
        EVT_POST(IRON_NOTIFY, e2int(iron_t::stateWorking));
        LOGI(T_CTRL, printf, "Engage work mode due to boost timeout of %u ms\n", _timeout.boost);
      }
    }

    default:;
  }
}

void IronController::_event_hndlr(void* handler, esp_event_base_t base, int32_t id, void* event_data){
  CTRL_LOGV(printf, "EVENT %s:%d\n", base, id);
  if ( base == SENSOR_DATA )
    return static_cast<IronController*>(handler)->_evt_sensors(base, id, event_data);

  if ( base == IRON_SET_EVT )
    return static_cast<IronController*>(handler)->_evt_commands(base, id, event_data);

  if ( base == IRON_GET_EVT )
    return static_cast<IronController*>(handler)->_evt_reqs(base, id, event_data);
}

void IronController::_evt_sensors(esp_event_base_t base, int32_t id, void* data){
  switch (static_cast<evt::iron_t>(id)){
    case evt::iron_t::motion :
      // update motion detect timestamp
      _xTicks.motion = xTaskGetTickCount();
  }
}

// process command events
void IronController::_evt_commands(esp_event_base_t base, int32_t id, void* data){
  switch (static_cast<evt::iron_t>(id)){

    // toggle working mode
    case evt::iron_t::workModeToggle : {
      // switch working mode on/off
      switch (_state){
        case ironState_t::idle :
        case ironState_t::standby :
        case ironState_t::suspend :
          // switch to working mode
          _state = ironState_t::working;
          // reset motion timer
          _xTicks.motion = xTaskGetTickCount();
          // notify other components
          LOGI(T_CTRL, println, "switch to working mode");
          EVT_POST(IRON_NOTIFY, e2int(iron_t::stateWorking));
          // set heater to working temperature
          EVT_POST_DATA(IRON_SET_EVT, e2int(iron_t::heaterTargetT), &_temp.working, sizeof(_temp.working));
          break;

        case ironState_t::boost :
        case ironState_t::working :
          // switch to idle mode
          _state = ironState_t::idle;
          // notify other components
          LOGI(T_CTRL, println, "switch to Idle mode");
          EVT_POST(IRON_NOTIFY, e2int(iron_t::stateIdle));
          break;
      }
      break;
    }

    // toggle boost mode
    case evt::iron_t::boostModeToggle : {
      switch (_state){
        case ironState_t::working : {
          // switch to boost mode
          _state = ironState_t::boost;
          // notify other components
          LOGI(T_CTRL, println, "switch to Boost mode");
          EVT_POST(IRON_NOTIFY, e2int(iron_t::stateBoost));
          // set heater to boost temperature
          int32_t t = _temp.working + _temp.boost;
          EVT_POST_DATA(IRON_SET_EVT, e2int(iron_t::heaterTargetT), &t, sizeof(t));
          _xTicks.boost = xTaskGetTickCount();
          break;
        }

        case ironState_t::boost :
          // switch to idle mode
          _state = ironState_t::working;
          // notify other components
          LOGI(T_CTRL, println, "switch to Work mode");
          EVT_POST(IRON_NOTIFY, e2int(iron_t::stateWorking));
          EVT_POST_DATA(IRON_SET_EVT, e2int(iron_t::heaterTargetT), &_temp.working, sizeof(_temp.working));
          break;
      }
      break;
    }

    // set work temperature (arrive from HID)
    case evt::iron_t::workTemp : {
      int32_t t = *reinterpret_cast<int32_t*>(data);
      if (t != _temp.working){
        _temp.working = *reinterpret_cast<int32_t*>(data);
        nvs_blob_write(T_IRON, T_temperatures, static_cast<void*>(&_temp), sizeof(Temperatures));
      }
      if (_state == ironState_t::working)
        EVT_POST_DATA(IRON_SET_EVT, e2int(iron_t::heaterTargetT), &_temp.working, sizeof(_temp.working));
      break;
    }

    // some
  }

}

void IronController::_evt_reqs(esp_event_base_t base, int32_t id, void* data){
  switch (static_cast<evt::iron_t>(id)){

    // req for working temperature
    case iron_t::workTemp :
      EVT_POST_DATA(IRON_STATE, e2int(iron_t::workTemp), &_temp.working, sizeof(_temp.working));
      break;
  }
}

// An instance of IronController
IronController espIron;
