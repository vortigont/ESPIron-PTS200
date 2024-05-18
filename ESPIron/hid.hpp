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
#include <mutex>
#include "common.hpp"
#include "evtloop.hpp"
#include "espasyncbutton.hpp"
#include "muipp_u8g2.h"
#include "lang/lang_en_us.h"


/**
 * @brief controls feedback events returned from VisualSet instance to IronHID class 
 * 
 */
enum class viset_evt_t {
  vsMainScreen,         // switch to Main work screen
  vsMenuMain,           // switch to Main Menu
  vsMenuTemperature,    // switch to Temperature setup menu
  vsMenuTimers,         // switch to Timers setup menu
  goBack                // switch to previous ViSet
};

/**
 * @brief A generic screen object instance
 * abstract class that represents some screen information,
 * a set of parameters to monitor and display, a menu, etc...
 * Specific implementation depends on derived class
 */
class VisualSet {

  // action button event handler
  esp_event_handler_instance_t _evt_btn_handler{nullptr};

  // encoder event handler
  esp_event_handler_instance_t _evt_enc_handler{nullptr};

  // event dispatcher
  static void _event_picker(void* arg, esp_event_base_t base, int32_t id, void* event_data);

protected:

  /**
   * @brief reference to pseudo-encoder object
   * ViSet can adjust it's properties to fit proper control
   * 
   */
  PseudoRotaryEncoder &encdr;

  /**
   * @brief reference to button object
   * ViSet can adjust it's properties to fit proper control
   * 
   */
  GPIOButton<ESPEventPolicy> &btn;

  // button events picker
  virtual void _evt_button(ESPButton::event_t e, const EventMsg* m){};
  // encoder events picker
  virtual void _evt_encoder(ESPButton::event_t e, const EventMsg* m){};

public:
  VisualSet(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder);
  virtual ~VisualSet();

  // draw on screen information
  virtual void drawScreen() = 0;
};


/**
 * @brief an object that manages button controls and Screen navigation
 * 
 */
class IronHID {

  // Display object - an instance of visual set that represents displayed info on a screen 
  std::unique_ptr<VisualSet> viset;

  viset_evt_t _cur_viset;

  // stack of ViSet's we go through (for goBack navigation)
  std::vector<viset_evt_t> _vistack;

  // screen redraw timer
  TimerHandle_t _tmr_display = nullptr;

  // ViSet mutex
  std::mutex _mtx;

  // event handler
  esp_event_handler_instance_t _evt_viset_handler = nullptr;

  // action button
  GPIOButton<ESPEventPolicy> _btn;

  // Two button pseudo-encoder
  PseudoRotaryEncoder _encdr;

  /**
   * @brief initialize screen
   * will set default VisualSet as a screen renderer
   * 
   */
  void _init_screen();

  void _viset_spawn(viset_evt_t v);

  void _viset_render();

public:
  // c-tor
  IronHID() : _encdr(BUTTON_DECR, BUTTON_INCR, LOW), _btn(BUTTON_ACTION, LOW) {};
  // d-tor
  ~IronHID();

  /**
   * @brief initialize HID,
   * attach to button events, init display class, etc...
   * 
   */
  void init();

  /**
   * @brief switch to another instance of VisualSet's object
   * this method will spawn a new instance of screen renderer depending on requested paramenter
   * 
   */
  void switchViSet(viset_evt_t v);


private:

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

  esp_event_handler_instance_t _evt_snsr_handler = nullptr;
  esp_event_handler_instance_t _evt_ntfy_handler = nullptr;
  esp_event_handler_instance_t _evt_set_handler = nullptr;
  esp_event_handler_instance_t _evt_state_handler = nullptr;

  // event dispatcher
  static void _event_picker(void* arg, esp_event_base_t base, int32_t id, void* event_data);

  // button events picker
  void _evt_button(ESPButton::event_t e, const EventMsg* m) override;
  // encoder events picker
  void _evt_encoder(ESPButton::event_t e, const EventMsg* m) override;

  // sensor events handler
  void _evt_sensor(int32_t id, void* data);

  // notify events handler
  void _evt_notify(int32_t id, void* data);

  // command events handler
  void _evt_cmd(int32_t id, void* data);

  // state events handler
  void _evt_state(int32_t id, void* data);

public:
  ViSet_MainScreen(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder);

  ~ViSet_MainScreen();

  // renders Main working screen
  void drawScreen() override;

};


/**
 * @brief generic class for menu objects
 * it will handle button and encoder events, screen refresh, etc...
 * menu functions must be implemented in derived classes
 * 
 */
class MuiMenu : public MuiPlusPlus, public VisualSet {

protected:
  // where to return to if this object gets quit event from MuiPlusPlus
  viset_evt_t parentvs{viset_evt_t::vsMainScreen};

  // screen refresh required
  bool _rr{true};

  // button events picker
  void _evt_button(ESPButton::event_t e, const EventMsg* m) override;
  // encoder events picker
  void _evt_encoder(ESPButton::event_t e, const EventMsg* m) override;

public:
  // c-tor
  MuiMenu(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder);

  /**
   * @brief handle that is called by screent refresh timer to redraw screen
   * 
   */
  void drawScreen() override;
};


/**
 * @brief this class will draw and navigate through a
 * different sections of configuration menu
 * 
 */
class ViSet_MainMenu : public MuiMenu {

  // menu builder function
  void _buildMenu();

  /**
   * @brief a callback method that hooked to MuiItem_U8g2_DynamicScrollList
   * it will pick index of a list item that represents desired submenu section
   * and send event to HID class to switch ViSet instance to another menu object
   * 
   */
  void _submenu_selector(size_t index);

public:
  // c-tor
  ViSet_MainMenu(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder) : MuiMenu(button, encoder) { _buildMenu(); }

};


/**
 * @brief temperature control menu
 * 
 */
class ViSet_TemperatureSetup : public MuiMenu {
  // configured temperatures
  //Temperatures _temp;
  // values that this section controls, -2 is for 'SaveLast' flag and 'back' button
  std::array<int32_t, menu_TemperatureOpts.size()-2> _temp;
  // save work flag
  bool save_work;

  // menu builder function
  void _buildMenu();

public:
  // c-tor
  ViSet_TemperatureSetup(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder);
  // d-tor
  ~ViSet_TemperatureSetup();

};


/**
 * @brief timeouts control menu
 * 
 */
class ViSet_TimeoutsSetup : public MuiMenu {
  // configured timeouts
  //IronTimeouts _timeout;
  std::array<unsigned, menu_TimeoutOpts.size()-1> _timeout;

  // callback for "Save work temp" option
  //void _cb_save_last_temp(size_t v);

  // menu builder function
  void _buildMenu();

public:
  // c-tor
  ViSet_TimeoutsSetup(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder);
  // d-tor
  ~ViSet_TimeoutsSetup();

};


// **************************
// Helper functions

/**
 * @brief will  create a string from provided numeric
 * 
 * @param v 
 * @return std::string 
 */
std::string numeric_to_string(int32_t v);



// **************************
// Our global instance of HID
extern IronHID hid;
