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
#include "common.hpp"
#include "evtloop.hpp"
#include "espasyncbutton.hpp"
#include "lang/dictionaries.h"
#include <memory>


/**
 * @brief A generic screen object instance
 * abstract class that represents some screen information,
 * a set of parameters to monitor and display, a menu, etc...
 * Specific implementation depends on derived class
 */
class VisualSet {
  esp_event_handler_instance_t _evt_handler = nullptr;

  // event dispatcher
  static void _event_picker(void* arg, esp_event_base_t base, int32_t id, void* event_data);

protected:
  lang_index lang{L_en_us};

  // sensor events picker
  virtual void _evt_sensor(int32_t id, void* event_data){};
  // notifications
  virtual void _evt_notify(int32_t id, void* event_data){};
  // commands
  virtual void _evt_cmd(int32_t id, void* data){};
  // commands
  virtual void _evt_state(int32_t id, void* data){};


public:
  VisualSet();
  virtual ~VisualSet();

};


/**
 * @brief Main Iron screen display
 * show working mode and basic sensors information
 * 
 */
class VisualSetMainScreen : public VisualSet {
  ironState_t _state{0};
  int32_t _wrk_temp{0}, _tip_temp{0};
  // sensor temp
  float _sns_temp{0};
  // input voltage
  uint32_t _vin{0};

  // renders Main working screen
  void _drawMainScreen();

  // sensor events handler
  void _evt_sensor(int32_t id, void* data) override;

  // notify events handler
  void _evt_notify(int32_t id, void* data) override;

  // command events handler
  void _evt_cmd(int32_t id, void* data) override;

  // state events handler
  void _evt_state(int32_t id, void* data) override;

public:
  VisualSetMainScreen();

};

/**
 * @brief and object that controls the screen and generates visual info
 * 
 */
class IronScreen {

  bool _hand_side;

  // an instance of visual oject that represents displayed info on a screen 
  std::unique_ptr<VisualSet> _viset;

public:

  void init();

};

/**
 * @brief an object that manages button controls and Screen navigation
 * 
 */
class IronHID {

  // Display object
  IronScreen _screen;

  // Two button pseudo-encoder
  PseudoRotaryEncoder _encdr;

  // action button
  GPIOButton<ESPEventPolicy> _btn;

  // action button menu
  ButtonCallbackMenu _menu;

  // action button event handler
  esp_event_handler_instance_t _btn_evt_handler = nullptr;

  // encoder event handler
  esp_event_handler_instance_t _enc_evt_handler = nullptr;

  // target temperature
  int32_t _wrk_temp;

  // encoder events executor, it works in cooperation with _menu object
  void _encoder_events(esp_event_base_t base, int32_t id, void* event_data);

  // init action button menu
  void _set_button_menu_callbacks();

  // switch between different menu modes for action button
  void _switch_buttons_modes(uint32_t level);

  /**
   * @brief button actions when iron is main working mode
   * 
   */
  void _menu_0_main_mode(ESPButton::event_t e, const EventMsg* m);

public:
  // c-tor
  IronHID();


  /**
   * @brief initialize HID,
   * attach to button events, init display class, etc...
   * 
   */
  void init(const Temperatures& t);

};


// Our global instance of HID
extern IronHID hid;
