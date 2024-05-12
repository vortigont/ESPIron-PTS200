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
#include "muipp_u8g2.h"

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
  // flag that shows a screen must be refreshed
  bool refresh_req{true};

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

  // MuiPlusPlus event sink, might be used in derrived classes
  virtual mui_event muipp_event(mui_event e){ return {}; };

  // draw on screen information
  virtual void drawScreen() = 0;
};


/**
 * @brief an object that manages button controls and Screen navigation
 * 
 */
class IronHID {
  // a set of availbale "screens"
  enum class vset_t {
    mainScreen = 0,
    configMenu
  };

  // Display object - an instance of visual set that represents displayed info on a screen 
  std::unique_ptr<VisualSet> viset;
  // screen redraw timer
  TimerHandle_t _tmr_display = nullptr;
  // flag that shows a screen must be refreshed
  //bool refresh_req{true};

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
  Temperatures _temp;

  bool _save_wrk_temp;

  // encoder events executor, it works in cooperation with _menu object
  void _encoder_events(esp_event_base_t base, int32_t id, void* event_data);

  // init action button menu
  void _set_button_menu_callbacks();

  // switch between different menu modes for action button
  void _switch_buttons_modes(uint32_t level);

  /**
   * @brief initialize screen
   * will set default VisualSet as a screen renderer
   * 
   */
  void _init_screen();

  /**
   * @brief button actions when iron is main working mode
   * 
   */
  void _menu_0_main_mode(ESPButton::event_t e, const EventMsg* m);

  void _menu_1_config_navigation(ESPButton::event_t e, const EventMsg* m);

public:
  // c-tor
  IronHID();


  /**
   * @brief initialize HID,
   * attach to button events, init display class, etc...
   * 
   */
  void init(const Temperatures& t);

  /**
   * @brief switch to another instance of VisualSet's object
   * this method will spawn a new instance of screen renderer depending on requested paramenter
   * 
   */
  void switchScreen(vset_t v = vset_t::mainScreen);



private:

  // MuiPP callback that sets save/not save work temperature flag
  void _cb_save_wrk_temp(size_t index);

};

/**
 * @brief Main Iron screen display
 * show working mode and basic sensors information
 * 
 */
class ViSet_MainScreen : public VisualSet {
  ironState_t _state{0};
  Temperatures _temp{0};
  int32_t _tip_temp{0};
  // sensor temp
  float _sns_temp{0};
  // input voltage
  uint32_t _vin{0};

  // renders Main working screen
  void drawScreen() override;

  // sensor events handler
  void _evt_sensor(int32_t id, void* data) override;

  // notify events handler
  void _evt_notify(int32_t id, void* data) override;

  // command events handler
  void _evt_cmd(int32_t id, void* data) override;

  // state events handler
  void _evt_state(int32_t id, void* data) override;

public:
  ViSet_MainScreen();

};

/**
 * @brief this class will draw and navigate through configuration menu
 * 
 */
class ViSet_ConfigurationMenu : public VisualSet, public MuiPlusPlus {

  void _build_menu();

  void _build_menu_temp_opts(muiItemId parent, muiItemId header, muiItemId footer);


public:
  ViSet_ConfigurationMenu();

  // MuiPlusPlus event sink, might be used in derrived classes
  mui_event muipp_event(mui_event e) override;

  void drawScreen() override;

};



// **************************
// Our global instance of HID
extern IronHID hid;
