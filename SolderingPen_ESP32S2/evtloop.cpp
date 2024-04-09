/*
    This file is a part of ESPIron-PTS200 project
    https://github.com/vortigont/ESPIron-PTS200

    Copyright © 2024 Emil Muratov (vortigont)

    ESPIron-PTS200 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/
#include <Arduino.h>
#include "esp32-hal.h"
#include "evtloop.hpp"

// LOGGING
#ifdef ARDUINO
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif

static const char* TAG = "evt";

// event base definitions
ESP_EVENT_DEFINE_BASE(SENSOR_DATA);
ESP_EVENT_DEFINE_BASE(IRON_SET_EVT);
ESP_EVENT_DEFINE_BASE(IRON_GET_EVT);
ESP_EVENT_DEFINE_BASE(IRON_NOTIFY);
ESP_EVENT_DEFINE_BASE(IRON_STATE);

namespace evt {

#define LOOP_EVT_Q_SIZE         16             // events loop queue size
#define LOOP_EVT_PRIORITY       1              // task priority is same as arduino's loop() to avoid extra context switches
#define LOOP_EVT_RUNNING_CORE   tskNO_AFFINITY // ARDUINO_RUNNING_CORE
#ifdef PTS200_DEBUG_LEVEL
 #define LOOP_EVT_STACK_SIZE     2048          // loop task stack size when debug is enabled, sprintf calls requires lots of mem
#else
 #define LOOP_EVT_STACK_SIZE     1536          // loop task stack size
#endif

void start(){
  if (hndlr) return;

  ESP_LOGI(TAG, "Cretating Event loop");

  esp_event_loop_args_t evt_cfg;
  evt_cfg.queue_size = LOOP_EVT_Q_SIZE;
  evt_cfg.task_name = "evt_loop";
  evt_cfg.task_priority = LOOP_EVT_PRIORITY;            // uxTaskPriorityGet(NULL) // same as parent
  evt_cfg.task_stack_size = LOOP_EVT_STACK_SIZE;
  evt_cfg.task_core_id = LOOP_EVT_RUNNING_CORE;

  //ESP_ERROR_CHECK(esp_event_loop_create(&levt_cfg, &loop_levt_h));
  esp_err_t err = esp_event_loop_create(&evt_cfg, &hndlr);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "event loop creation failed!");
  }
}

void stop(){ esp_event_loop_delete(hndlr); hndlr = nullptr; };

esp_event_loop_handle_t get_hndlr(){ return hndlr; };

void debug_hndlr(void* handler_args, esp_event_base_t base, int32_t id, void* event_data){
  Serial.printf("evt tracker: %s:%d\n", base, id);
}

void debug(){
    ESP_ERROR_CHECK( esp_event_handler_instance_register_with(hndlr, ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, debug_hndlr, NULL, nullptr) );
}

} // namespace evt
