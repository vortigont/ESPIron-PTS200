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
#include "esp_event.h"
#include <type_traits>

// helper macro to reduce typing
#define EVT_POST(event_base, event_id) esp_event_post_to(evt::get_hndlr(), event_base, event_id, NULL, 0, portMAX_DELAY)
#define EVT_POST_DATA(event_base, event_id, event_data, data_size) esp_event_post_to(evt::get_hndlr(), event_base, event_id, event_data, data_size, portMAX_DELAY)

// ESP32 event loop defines
ESP_EVENT_DECLARE_BASE(SENSOR_DATA);        // events coming from different sensors, i.e. temperature, voltage, orientation, etc...
ESP_EVENT_DECLARE_BASE(IRON_SET_EVT);       // ESPIron setter Commands events base (in reply to this command, an IRON_STATE_EVT could be generated)
ESP_EVENT_DECLARE_BASE(IRON_GET_EVT);       // ESPIron getter Commands events base (in reply to this command, an IRON_STATE_EVT could be generated)
ESP_EVENT_DECLARE_BASE(IRON_NOTIFY);        // ESPIron notification events base (those events are published when some state or mode changes due to any commands or component's logic)
ESP_EVENT_DECLARE_BASE(IRON_STATE);         // ESPIron State publishing events base (those events are published on IRON_GET_EVT requests on demand)
ESP_EVENT_DECLARE_BASE(IRON_VISET);         // ESPIron VisualSet HID events

// cast enum to int
template <class E>
constexpr std::common_type_t<int, std::underlying_type_t<E>>
e2int(E e) {
    return static_cast<std::common_type_t<int, std::underlying_type_t<E>>>(e);
}

// ESPIron Event Loop
namespace evt {

// ESPIron events
enum class iron_t:int32_t {
  noop = 0,                 // 0-9 are reserved for something extraordinary

  // Sensors data 100-199
  motion=100,               // motion detected from GyroSensor
  vin=110,                  // Vin voltage in millvolts, parameter uint32_t
  tiptemp=120,              // current Tip temperature, parameter int32_t
  acceltemp=121,            // accelerometer chip temperature, parameter float


  // Commands
  sensorsReload = 200,      // reload configuration for any sensors available
  heaterTargetT,            // set heater target temperature, parameter int32_t
  workTemp,                 // set working temperature, parameter int32_t
  workModeToggle,           // toggle working mode on/off
  boostModeToggle,          // toggle boost mode on/off

  reloadTemp,               // reload temperature configuration

  // State notifications
  stateWorking = 300,
  stateStandby,             // iron controller switched to 'Standby' mode
  stateIdle,
  stateSuspend,
  stateBoost,               // iron controller switched to 'Boost' mode, parameter uint32_t - seconds left to disable boost mode
  stateSetup,
  stateNoTip,
  tipEject,                 // sent by heater when it looses the tip sense
  tipInsert,                // sent by heater when detect tip sensor

  // END
  noop_end                  // stub
};


  // Event loop handler
  static esp_event_loop_handle_t hndlr = nullptr;

  /**
   * @brief Start ESPIron's event loop task
   * this loop will manage events handling amoung application's components and controls
   *
   * @return esp_event_loop_handle_t* a pointer to loop handle
   */
  void start();

  /**
   * @brief Stops ESPIron's event loop task
   *
   * @return esp_event_loop_handle_t* a pointer to loop handle
   */
  void stop();

  /**
   * @brief Get event loop hndlr ptr
   * 
   * @return esp_event_loop_handle_t 
   */
  esp_event_loop_handle_t get_hndlr();

  // subscribe to all events on a bus and print debug messages
  void debug();

} // namespace evt
