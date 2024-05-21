/*
    This file is a part of ESPIron-PTS200 project
    https://github.com/vortigont/ESPIron-PTS200

    Copyright © 2024 Emil Muratov (vortigont)

    ESPIron-PTS200 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "hid.hpp"
#include "nvs.hpp"
#include "const.h"
#include <sstream>
#include "log.h"

/*
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
*/
#include <Wire.h>

// font
#include "PTS200_16.h"
//#include "Languages.h"

// Setup u8g object depending on OLED controller
#if defined(SSD1306)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/22, /* data=*/21);
#elif defined(SH1107)
U8G2_SH1107_64X128_F_HW_I2C u8g2(U8G2_R1, SH1107_RST_PIN);
#else
#error Wrong OLED controller type!
#endif

#define TIMER_DISPLAY_REFRESH     100   // 10 fps max   (screen redraw is ~35 ms)

#define X_OFFSET_TIP_TEMP         50
#define Y_OFFSET_TIP_TEMP         18

#define OFFSET_TARGET_TEMP_1      78
#define OFFSET_TARGET_TEMP_2      95
#define Y_OFFSET_SNS_TEMP         50
#define X_OFFSET_VIN              94
#define Y_OFFSET_VIN              50


// shortcut type aliases
using ESPButton::event_t;
using evt::iron_t;

//  *********************
//  ***   IronHID     ***

IronHID::~IronHID(){
  if (_evt_viset_handler){
      esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_VISET, ESP_EVENT_ANY_ID, _evt_viset_handler);
    _evt_viset_handler = nullptr;
  }  

  if (_evt_ntfy_handler){
      esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_NOTIFY, e2int( iron_t::stateSuspend ), _evt_ntfy_handler);
    _evt_ntfy_handler = nullptr;
  }

  if (_tmr_display)
    xTimerDelete( _tmr_display, portMAX_DELAY );
}

void IronHID::_init_screen(){
#ifndef NO_DISPLAY
  u8g2.initDisplay();
  u8g2.begin();
  u8g2.sendF("ca", 0xa8, 0x3f);
  u8g2.enableUTF8Print();
#endif

  // subscribe to ViSet events
  esp_event_handler_instance_register_with(
    evt::get_hndlr(),
    IRON_VISET,
    ESP_EVENT_ANY_ID,
    // VisualSet switching events
    [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<IronHID*>(self)->switchViSet(static_cast<viset_evt_t>(id)); },
    this,
    &_evt_viset_handler
  );

  // setup timer that will redraw screen periodically
  if (!_tmr_display){
    _tmr_display = xTimerCreate("scrT",
                              pdMS_TO_TICKS(TIMER_DISPLAY_REFRESH),
                              pdTRUE,
                              static_cast<void*>(this),
                              [](TimerHandle_t h) { static_cast<IronHID*>(pvTimerGetTimerID(h))->_viset_render(); }
                            );
    if (_tmr_display)
      xTimerStart( _tmr_display, portMAX_DELAY );
  }

/*  TBD
  if(_hand_side){
    u8g2.setDisplayRotation(U8G2_R3);
  }else{
    u8g2.setDisplayRotation(U8G2_R1);
  }
*/

}

void IronHID::switchViSet(viset_evt_t v){
  // check if return to prev screen has been requested
  if (v == viset_evt_t::goBack){
    //return _viset_spawn(viset_evt_t::vsMainScreen);
    if (_vistack.size()){
      _cur_viset = _vistack.back();
      _vistack.pop_back();
      return _viset_spawn(_cur_viset);
    } else  // nowhere to return, go main screen ViSet
      v = viset_evt_t::vsMainScreen;
  } 

  if (v == viset_evt_t::vsMainScreen){
    // switching to mainScreen resets stack
    _vistack.clear();
    _cur_viset = viset_evt_t::vsMainScreen;
  } else {
    // save current ViSet to stack
    _vistack.push_back(_cur_viset);
  }
  _cur_viset = v;
  _viset_spawn(_cur_viset);
  LOGI(T_HID, printf, "heap:%u\n", ESP.getFreeHeap()/1024);
}

void IronHID::_viset_spawn(viset_evt_t v){
  LOGI(T_HID, printf, "switch ViSet:%u\n", e2int(v));
  // obtain mutex lock while switching the object to prevent concurent operations from other threads
  std::lock_guard<std::mutex> mylock(_mtx);

  switch(v){
    case viset_evt_t::vsMainScreen :
      viset = std::make_unique<ViSet_MainScreen>(_btn, _encdr);
      break;
    case viset_evt_t::vsMenuMain :
      viset = std::make_unique<ViSet_MainMenu>(_btn, _encdr);
      break;
    case viset_evt_t::vsMenuTemperature :
      viset = std::make_unique<ViSet_TemperatureSetup>(_btn, _encdr);
      break;
    case viset_evt_t::vsMenuTimers :
      viset = std::make_unique<ViSet_TimeoutsSetup>(_btn, _encdr);
      break;
  };
}

void IronHID::init(){
  // link our event loop with ESPButton's loop
  ESPButton::set_event_loop_hndlr(evt::get_hndlr());

  // create default ViSet with Main Work screen
  switchViSet(viset_evt_t::vsMainScreen);

  // initialize screen
  _init_screen();

  // subscribe to notification events
  if (!_evt_ntfy_handler){
    esp_event_handler_instance_register_with(
      evt::get_hndlr(),
      IRON_NOTIFY,
      e2int( iron_t::stateSuspend ),    // I need only 'suspend' for now
      // notification on suspend
      [](void* self, esp_event_base_t base, int32_t id, void* data) { u8g2.sleepOn(); },    // suspend display
      this,
      &_evt_ntfy_handler
    );

  }


  // enable middle button
  _btn.enable();
  // enable 'encoder' buttons
  _encdr.begin();
}

void IronHID::_viset_render(){
  std::unique_lock<std::mutex> lock(_mtx, std::defer_lock);
  // try to obtain mutex lock, if mutex is not available then simply skip this run and return later
  if (!lock.try_lock())
    return;

  viset->drawScreen();
}

// ***** VisualSet - Generic *****
VisualSet::VisualSet(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder) : btn(button), encdr(encoder) {
  // subscribe to button events
  esp_event_handler_instance_register_with(evt::get_hndlr(), EBTN_EVENTS, ESP_EVENT_ANY_ID, VisualSet::_event_picker, this, &_evt_btn_handler);

  // subscribe to encoder events
  esp_event_handler_instance_register_with(evt::get_hndlr(), EBTN_ENC_EVENTS, ESP_EVENT_ANY_ID, VisualSet::_event_picker, this, &_evt_enc_handler);

/*
  // subscribe to all events on a bus
  ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
                    evt::get_hndlr(),
                    ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID,
                    VisualSet::_event_picker,
                    this, &_evt_handler)
  );
*/
}

VisualSet::~VisualSet(){
  LOGD(T_HID, println, "~VisualSet d-tor");
  // unsubscribe from an event bus
  if (_evt_btn_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), EBTN_EVENTS, ESP_EVENT_ANY_ID, _evt_btn_handler);
    _evt_btn_handler = nullptr;
  }
  if (_evt_enc_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), EBTN_ENC_EVENTS, ESP_EVENT_ANY_ID, _evt_enc_handler);
    _evt_enc_handler = nullptr;
  }
}

void VisualSet::_event_picker(void* arg, esp_event_base_t base, int32_t id, void* data){
  //LOGV(printf, "VisualSet::_event_picker %s:%d\n", base, id);

  // button events
  if (base == EBTN_EVENTS){
    // pick events only for "enter" gpio putton
    if (reinterpret_cast<const EventMsg*>(data)->gpio == BUTTON_ACTION){
      LOGV(T_HID, printf, "btn event:%d, gpio:%d\n", id, reinterpret_cast<EventMsg*>(data)->gpio);
      static_cast<VisualSet*>(arg)->_evt_button(ESPButton::int2event_t(id), reinterpret_cast<const EventMsg*>(data));
      //static_cast<VisualSet*>(self)->_btn_menu.handleEvent(ESPButton::int2event_t(id), reinterpret_cast<const EventMsg*>(data));
    }
    return;
  }

  // encoder events
  if (base == EBTN_ENC_EVENTS){
    LOGV(T_HID, printf, "enc event:%d, cnt:%d\n", id, reinterpret_cast<EventMsg*>(data)->cntr);
    // pick encoder events and pass it to _menu member
    static_cast<VisualSet*>(arg)->_evt_encoder(ESPButton::int2event_t(id), reinterpret_cast<const EventMsg*>(data));
    return;
  }
}


// ***** VisualSet - Main Screen *****
ViSet_MainScreen::ViSet_MainScreen(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder) : VisualSet(button, encoder) {
  //LOGV(printf, "ViSet_MainScreen::_event_picker %s:%d\n", base, id);

  // subscribe to sensor events
  esp_event_handler_instance_register_with(evt::get_hndlr(), SENSOR_DATA, ESP_EVENT_ANY_ID,
            [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<ViSet_MainScreen*>(self)->_evt_sensor(id, data); },
            this, &_evt_snsr_handler);
  // subscribe to notify events
  esp_event_handler_instance_register_with(evt::get_hndlr(), IRON_NOTIFY, ESP_EVENT_ANY_ID,
            [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<ViSet_MainScreen*>(self)->_evt_notify(id, data); },
            this, &_evt_ntfy_handler);
  // subscribe to notify events
  esp_event_handler_instance_register_with(evt::get_hndlr(), IRON_SET_EVT, ESP_EVENT_ANY_ID,
            [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<ViSet_MainScreen*>(self)->_evt_cmd(id, data); },
            this, &_evt_set_handler);
  // subscribe to notify events
  esp_event_handler_instance_register_with(evt::get_hndlr(), IRON_STATE, ESP_EVENT_ANY_ID,
            [](void* self, esp_event_base_t base, int32_t id, void* data) { static_cast<ViSet_MainScreen*>(self)->_evt_state(id, data); },
            this, &_evt_state_handler);


  // configure button and encoder
  btn.deactivateAll();
  btn.enableEvent(event_t::click);
  btn.enableEvent(event_t::longPress);
  btn.enableEvent(event_t::multiClick);
  // set encoder to control working temperature
  encdr.reset();
  encdr.setCounter(TEMP_DEFAULT, TEMP_STEP, TEMP_MIN, TEMP_MAX);
  encdr.setMultiplyFactor(2);

  // request working temperature from IronController
  EVT_POST(IRON_GET_EVT, e2int(iron_t::workTemp));
}

ViSet_MainScreen::~ViSet_MainScreen(){
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), SENSOR_DATA, ESP_EVENT_ANY_ID, _evt_snsr_handler);
  _evt_snsr_handler = nullptr;
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_NOTIFY, ESP_EVENT_ANY_ID, _evt_ntfy_handler);
  _evt_ntfy_handler = nullptr;
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_SET_EVT, ESP_EVENT_ANY_ID, _evt_set_handler);
  _evt_set_handler = nullptr;
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), IRON_STATE, ESP_EVENT_ANY_ID, _evt_state_handler);
  _evt_state_handler = nullptr;
  LOG(println, "d-tor ViSet_MainScreen");
}

void ViSet_MainScreen::drawScreen(){
  u8g2.clearBuffer();

  u8g2.setFont(MAINSCREEN_FONT);
    //u8g2.setFont(PTS200_16);

  u8g2.setFontPosTop();

  // draw status of heater 绘制加热器状态
  u8g2.setCursor(0, 0 + SCREEN_OFFSET);
  switch (_state){
    case ironState_t::idle :
      u8g2.print(dictionary[D_idle]);
      break;
    case ironState_t::working :
      u8g2.print(dictionary[D_heating]);
      break;
    case ironState_t::standby :
      u8g2.print(dictionary[D_standby]);
      break;
    case ironState_t::boost :
      u8g2.print(dictionary[D_boost]);
      break;
    case ironState_t::notip :
      u8g2.print(dictionary[D_notip]);
      break;

    default:;
  }

  // print working temperature in the upper right corner
  //    u8g2.drawUTF8(0, 0 + SCREEN_OFFSET, "设温:");
  //u8g2.drawUTF8(OFFSET_TARGET_TEMP_1, 0 + SCREEN_OFFSET, dictionary[D_set_t]);   // target temperature
  //u8g2.setFont(u8g2_font_unifont_t_chinese3);
  u8g2.setCursor(OFFSET_TARGET_TEMP_1, 0 + SCREEN_OFFSET);
  //u8g2.drawUTF8(OFFSET_TARGET_TEMP_1, 0 + SCREEN_OFFSET, "T:");   // target temperature  
  //u8g2.setCursor(OFFSET_TARGET_TEMP_2, 0 + SCREEN_OFFSET);
  u8g2.print("T:");
  // print target temperature depending on Iron work state
  switch(_state){
    case ironState_t::standby :
      u8g2.print(_temp.standby, 0);
      break;
    case ironState_t::boost :
      u8g2.print(_temp.working + _temp.boost, 0);
      break;
    default:
      u8g2.print(_temp.working, 0);
      break;
  }
  u8g2.print("C");

/*
  if(lang != 0){
    u8g2.setFont(PTS200_16);
  }

  if (_tip_temp > TEMP_NOTIP)
    u8g2.print(dictionary[D_error]);
  else if (inOffMode)
    u8g2.print(txt_off[language]);
  else if (inSleepMode)
    u8g2.print(txt_sleep[language]);
  else if (inBoostMode)
    u8g2.print(txt_boost[language]);
  else if (isWorky)
    u8g2.print(txt_worky[language]);
  else if (heater.getCurrentTemp() - heater.getTargetTemp() < PID_ENGAGE_DIFF)
    u8g2.print(txt_on[language]);
  else
    u8g2.print(txt_hold[language]);

  u8g2.setFont(u8g2_font_unifont_t_chinese3);
*/

  // casing internal temperature sensor
  u8g2.setCursor(0, Y_OFFSET_SNS_TEMP);
  u8g2.print(_sns_temp, 1);
  u8g2.print("C");
  
  // input voltage
  u8g2.setCursor(X_OFFSET_VIN, Y_OFFSET_VIN);
  u8g2.print(_vin/1000.0, 1);  // convert mv in V
  u8g2.print("V");

  // draw current tip temperature 绘制当前温度
  u8g2.setFont(u8g2_font_freedoomr25_tn);
  u8g2.setFontPosTop();
  u8g2.setCursor(X_OFFSET_TIP_TEMP, Y_OFFSET_TIP_TEMP);
  if (_tip_temp > TEMP_NOTIP)
    u8g2.print("Err");
  else
    u8g2.printf("%03d", _tip_temp);

/*
  // draw current temperature in big figures 用大数字绘制当前温度
  u8g2.setFont(u8g2_font_fub42_tn);
  u8g2.setFontPosTop();
  u8g2.setCursor(15, 10);
  if (_tip_temp > TEMP_NOTIP)
    u8g2.print("Err");
  else
    u8g2.printf("%03d", _tip_temp);
*/

  u8g2.sendBuffer();
}

void ViSet_MainScreen::_evt_sensor(int32_t id, void* data){
  switch(static_cast<evt::iron_t>(id)){
    case evt::iron_t::tiptemp :
      _tip_temp = *reinterpret_cast<int32_t*>(data);
      break;
    case evt::iron_t::vin :
      _vin = *reinterpret_cast<uint32_t*>(data);
      break;
    case evt::iron_t::acceltemp :
      _sns_temp = *reinterpret_cast<float*>(data);
      break;
  }
  //_drawMainScreen();
}

void ViSet_MainScreen::_evt_notify(int32_t id, void* data){
  switch(static_cast<evt::iron_t>(id)){
    case evt::iron_t::stateIdle :
      _state = ironState_t::idle;
    break;
    case evt::iron_t::stateWorking :
      _state = ironState_t::working;
    break;
    case evt::iron_t::stateStandby :
      _state = ironState_t::standby;
    break;
    case evt::iron_t::stateBoost :
      _state = ironState_t::boost;
    break;
  }
}

void ViSet_MainScreen::_evt_cmd(int32_t id, void* data){
  switch(static_cast<evt::iron_t>(id)){
    case evt::iron_t::workTemp :
      _temp.working = *reinterpret_cast<int32_t*>(data);
      encdr.setCounter(_temp.working, TEMP_STEP, TEMP_MIN, TEMP_MAX);
    break;
  }
}

void ViSet_MainScreen::_evt_state(int32_t id, void* data){
  switch(static_cast<evt::iron_t>(id)){
    case evt::iron_t::workTemp :
      _temp.working = *reinterpret_cast<int32_t*>(data);
      encdr.setCounter(_temp.working, TEMP_STEP, TEMP_MIN, TEMP_MAX);
    break;
  }
}

void ViSet_MainScreen::_evt_button(ESPButton::event_t e, const EventMsg* m){
  // actions for middle button when iron is main working mode
  LOGD(T_HID, printf, "Button main screen e:%d, cnt: %d\n", e, m->cntr);
  switch(e){
    // Use click event to toggle iron working mode on/off
    case event_t::click :
      EVT_POST(IRON_SET_EVT, e2int(iron_t::workModeToggle));
      break;

    // use longPress to enter configuration menu
    case event_t::longPress :
      EVT_POST(IRON_VISET, e2int(viset_evt_t::vsMenuMain));
      break;

    // pick multiClicks
    case event_t::multiClick :
      // doubleclick to toggle boost mode
      if (m->cntr == 2)
        EVT_POST(IRON_SET_EVT, e2int(iron_t::boostModeToggle));
      break;
  }
}

void ViSet_MainScreen::_evt_encoder(ESPButton::event_t e, const EventMsg* m){
  // main Iron screen mode - encoder controls Iron working temperature
  _temp.working = m->cntr; // reinterpret_cast<EventMsg*>(event_data)->cntr;
  EVT_POST_DATA(IRON_SET_EVT, e2int(iron_t::workTemp), &_temp.working, sizeof(_temp.working));
}



// ***** MuiMenu Generics *****

MuiMenu::MuiMenu(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder) : VisualSet(button, encoder) {
  // set buttons and encoder
  btn.deactivateAll();
  btn.enableEvent(event_t::click);
  btn.enableEvent(event_t::longPress);
  // set encoder to control cursor
  encdr.reset();
  //_encdr.setCounter(_temp.working, 5, TEMP_MIN, TEMP_MAX);
  //_encdr.setMultiplyFactor(2);
};

// actions for middle button in main Configuration menu
void MuiMenu::_evt_button(ESPButton::event_t e, const EventMsg* m){

  LOGD(T_HID, printf, "_evt_button:%u, cnt:%d\n", e2int(e), m->cntr);
  switch(e){
    // Use click event to send 'enter/ok' to MUI menu
    case event_t::click :{
      auto e = muiEvent( mui_event(mui_event_t::enter) );
      // signal switch to previous ViSet if quitMenu event received
      if (e.eid == mui_event_t::quitMenu){
        EVT_POST(IRON_VISET, e2int(parentvs));
      }
      break;
    }

    // use longPress to escape current item
    case event_t::longPress : {
      auto e = muiEvent( mui_event(mui_event_t::escape) );
      // signal switch to Main Work Screen if quitMenu event received, log press will always quit Menu to main screen
      if (e.eid == mui_event_t::quitMenu){
        EVT_POST(IRON_VISET, e2int(viset_evt_t::vsMainScreen));
      }
      break;
    }
  }

  // redraw screen
  _rr = true;
}

// encoder events picker
void MuiMenu::_evt_encoder(ESPButton::event_t e, const EventMsg* m){
  LOGD(T_HID, printf, "_evt_encoder:%u, cnt:%d\n", e2int(e), m->cntr);
  // I do not need counter value here (for now), just figure out if it was increment or decrement via gpio which triggered and event
  if (m->gpio == BUTTON_INCR){
    muiEvent( {mui_event_t::moveDown, 0, nullptr} );
  } else {
    muiEvent( {mui_event_t::moveUp, 0, nullptr} );
  }

  // redraw screen
  _rr = true;
}

void MuiMenu::drawScreen(){
  if (!_rr) return;

  Serial.printf("st:%lu\n", millis());
  u8g2.clearBuffer();
  // call Mui renderer
  render();
  u8g2.sendBuffer();
  Serial.printf("en:%lu\n", millis());
  // take a screenshot
  //u8g2.writeBufferXBM(Serial);
  _rr = false;
}


//  **************************************
//  ***   Main Configuration Menu      ***

void ViSet_MainMenu::_buildMenu(){
  LOGD(T_HID, println, "Build Main Menu");

  // create page with Settings options list
  muiItemId root_page = makePage(dictionary[D_Settings]);

  // create page "Settings->Timers"
  muiItemId page_set_time = makePage(menu_MainConfiguration.at(1), root_page);

  // create page "Settings->Tip"
  muiItemId page_set_tip = makePage(menu_MainConfiguration.at(2), root_page);

  // create page "Settings->Power"
  muiItemId page_set_pwr = makePage(menu_MainConfiguration.at(3), root_page);

  // create page "Settings->Info"
  muiItemId page_set_info = makePage(menu_MainConfiguration.at(4), root_page);

  // generate idx for "Page title" item
  muiItemId page_title_id = nextIndex();
  // create "Page title" item, add it to menu and bind to "Main Settings" page
  addMuippItem(new MuiItem_U8g2_PageTitle (u8g2, page_title_id, PAGE_TITLE_FONT ), root_page);

  // bind  "Page title" item with "Settings->Timers" page
  addItemToPage(page_title_id, page_set_time);
  // bind  "Page title" item with "Settings->Tip" page
  addItemToPage(page_title_id, page_set_tip);
  // bind  "Page title" item with "Settings->Power" page
  addItemToPage(page_title_id, page_set_pwr);
  // bind  "Page title" item with "Settings->Info" page
  addItemToPage(page_title_id, page_set_info);


  // generate idx for "Back Button" item
  muiItemId bbuton_id = nextIndex();
  // create "Back Button" item, bind it to "Settings->Timers" page
  addMuippItem( new MuiItem_U8g2_BackButton (u8g2, bbuton_id, dictionary[D_return], MAIN_MENU_FONT1),
    page_set_time
  );

  // bind  "Back Button" item with "Settings->Tip" page
  addItemToPage(bbuton_id, page_set_tip);
  // bind  "Back Button" item with "Settings->Power" page
  addItemToPage(bbuton_id, page_set_pwr);
  // bind  "Back Button" item with "Settings->Info" page
  addItemToPage(bbuton_id, page_set_info);

  // create and add to main page a list with settings selection options
  muiItemId scroll_list_id = nextIndex();

  // Main menu dynamic scroll list
  MuiItem_U8g2_DynamicScrollList *menu = new MuiItem_U8g2_DynamicScrollList(u8g2, scroll_list_id,
    [](size_t index){ /* Serial.printf("idx:%u\n", index); */ return menu_MainConfiguration.at(index); },   // this lambda will feed localized strings to the MuiItem list builder class
    [](){ return menu_MainConfiguration.size(); },                  // list size callback
    [this](size_t idx){ _submenu_selector(idx); },                  // action callback
    MAIN_MENU_Y_SHIFT, 3,                                           // offset for each line of text and total number of lines in menu
    MAIN_MENU_X_OFFSET, MAIN_MENU_Y_OFFSET,                         // x,y cursor
    MAIN_MENU_FONT2, MAIN_MENU_FONT2
  );

  // last item in the menu acts as "back" button
  menu->listopts.back_on_last = true;
  // "back" event should be replaced with "Quit menu"
  menu->on_escape = mui_event_t::quitMenu;
  // move menu object into MuiPP page
  addMuippItem(menu, root_page);
  // Assign menu as autoselecting item
  pageAutoSelect(root_page, scroll_list_id);

  // start menu from page mainmenu
  menuStart(root_page);
}

void ViSet_MainMenu::_submenu_selector(size_t index){
  LOGD(T_HID, printf, "_submenu_selector:%u\n", index);
  // index here matches elements of menu_MainConfiguration array
  switch (index){
    case 0 :  // temp settings
      EVT_POST(IRON_VISET, e2int(viset_evt_t::vsMenuTemperature));
      break;
    case 1 :  // timers settings
      EVT_POST(IRON_VISET, e2int(viset_evt_t::vsMenuTimers));
      break;
    default:  // for unknown items quit to main screen
      EVT_POST(IRON_VISET, e2int(viset_evt_t::vsMainScreen));
  }
}


//  **************************************
//  ***   Temperature Control Menu     ***

ViSet_TemperatureSetup::ViSet_TemperatureSetup(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder) : MuiMenu(button, encoder){
  LOGD(T_HID, println, "Build temp menu");
  // go back to prev viset on exit
  parentvs = viset_evt_t::goBack;
  Temperatures t;
  // load temperature values from NVS
  nvs_blob_read(T_IRON, T_temperatures, &t, sizeof(decltype(t)));

  _temp.at(0) = t.deflt;
  _temp.at(1) = t.standby;
  _temp.at(2) = t.boost;
  save_work = t.savewrk;
  _buildMenu();
}

ViSet_TemperatureSetup::~ViSet_TemperatureSetup(){
  LOGD(T_HID, println, "d-tor TemperatureSetup");
  // save temp settings to NVS
  Temperatures t;
  nvs_blob_read(T_IRON, T_temperatures, &t, sizeof(decltype(t)));
  t.deflt   =  _temp.at(0);
  t.standby =  _temp.at(1);
  t.boost   =  _temp.at(2);
  t.savewrk =  save_work;
  nvs_blob_write(T_IRON, T_temperatures, &t, sizeof(decltype(t)));
  // send command to reload temp settings
  EVT_POST(IRON_SET_EVT, e2int(iron_t::reloadTemp));
}

void ViSet_TemperatureSetup::_buildMenu(){
  // default timeout values
  constexpr std::array<int32_t, menu_TemperatureOpts.size()-2> def_temp_min = {
    TEMP_MIN,
    TEMP_STANDBY_MIN,
    TEMP_BOOST_MIN
  };

  constexpr std::array<int32_t, menu_TemperatureOpts.size()-2> def_temp_max = {
    TEMP_MAX,
    TEMP_STANDBY_MAX,
    TEMP_BOOST_MAX
  };

  constexpr std::array<int32_t, menu_TemperatureOpts.size()-2> def_temp_step = {
    TEMP_STEP,
    TEMP_STANDBY_STEP,
    TEMP_BOOST_STEP
  };

  // create root page "Settings->Temperature"
  muiItemId root_page = makePage(menu_MainConfiguration.at(0));

  // create "Page title" item and assign it to root page
  addMuippItem(new MuiItem_U8g2_PageTitle (u8g2, nextIndex(), PAGE_TITLE_FONT_SMALL ),  root_page );


  // create and add to main page a list with settings selection options
  muiItemId scroll_list_id = nextIndex();

  // Temperature menu options dynamic scroll list
  auto list = std::make_shared< MuiItem_U8g2_DynamicScrollList>(u8g2, scroll_list_id,
    [](size_t index){ /* Serial.printf("idx:%u\n", index); */ return menu_TemperatureOpts.at(index); },   // this lambda will feed localized strings to the MuiItem list builder class
    [](){ return menu_TemperatureOpts.size(); },
    nullptr,                                                        // action callback
    MAIN_MENU_Y_SHIFT, MAIN_MENU_ROWS,                              // offset for each line of text and total number of lines in menu
    MAIN_MENU_X_OFFSET, MAIN_MENU_Y_OFFSET,                         // x,y cursor
    MAIN_MENU_FONT3, MAIN_MENU_FONT3
  );

  // dynamic list will act as page selector
  list->listopts.page_selector = true;
  list->listopts.back_on_last = true;
  list->on_escape = mui_event_t::quitMenu;                          // this is the only active element on a page, so I quit on escape
  // move menu object into MuiPP page
  addMuippItem(list, root_page);
  // this is the only active Item on a page, so outselect it
  pageAutoSelect(root_page, scroll_list_id);


  // create "Page title" item, I'll add it to other pages later on
  muiItemId title2_id = nextIndex();
  addMuippItem( std::make_shared< MuiItem_U8g2_PageTitle > (u8g2, title2_id, PAGE_TITLE_FONT) );

  // autogenerate items for each temperature setting value
  for (auto i = 0; i != _temp.size(); ++i){
    // make page, bound to root page
    muiItemId page = makePage(menu_TemperatureOpts.at(i), root_page);
    // add page title element
    addItemToPage(title2_id, page);
    // create num slider Mui Item
    muiItemId idx = nextIndex();
    auto hslide = std::make_shared< MuiItem_U8g2_NumberHSlide<int32_t> >(
      u8g2, idx,
      nullptr,   // label
      _temp.at(i),
      def_temp_min[i], def_temp_max[i], def_temp_step[i],
      numeric_to_string,
      nullptr, nullptr, nullptr,
      NUMERIC_FONT1, MAIN_MENU_FONT2,
      u8g2.getDisplayWidth()/2, u8g2.getDisplayHeight()/2, NUMBERSLIDE_X_OFFSET
    );
    // since this item is the only active on the page, return to prev page on unselect
    hslide->on_escape = mui_event_t::prevPage;
    // add num slider to page
    addMuippItem(hslide, page);
    // make this item autoselected on this page
    pageAutoSelect(page, idx);

/*
    // todo: add celsius/farenh label
    // value label hint position in the bottom center of screen
    auto txt = new MuiItem_U8g2_StaticText(u8g2, nextIndex(), def_timeouts_label[i], PAGE_TITLE_FONT, u8g2.getDisplayWidth()/2, u8g2.getDisplayHeight());
    txt->h_align = text_align_t::bottom;
    txt->v_align = text_align_t::center;
    addMuippItem(txt, page);
*/
  }

  // ***
  // page "Save last Work temp" checkbox
  muiItemId page = makePage(menu_TemperatureOpts.at(menu_TemperatureOpts.size()-2), root_page);
  // add page Title
  addItemToPage(title2_id, page);
  // create checkbox item
  addMuippItem(
    new MuiItem_U8g2_CheckBox(u8g2, nextIndex(), dictionary[D_SaveLast_box], save_work, [this](size_t v){ save_work = v; }, MAINSCREEN_FONT, 0, 35),
    page);

  // create text hint
  addMuippItem(new MuiItem_U8g2_StaticText(u8g2, nextIndex(), dictionary[D_SaveLast_hint], MAIN_MENU_FONT1, 0, 45),
    page);

  // create "Back Button" item
  addMuippItem(new MuiItem_U8g2_BackButton(u8g2, nextIndex(), dictionary[D_return], MAIN_MENU_FONT1),
    page);

  // start menu from root page
  menuStart(root_page); 
}



//  **************************************
//  ***   Timeouts Control Menu        ***

ViSet_TimeoutsSetup::ViSet_TimeoutsSetup(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder) : MuiMenu(button, encoder){
  // go back to prev viset on exit
  parentvs = viset_evt_t::goBack;
  IronTimeouts t;
  // load temperature values from NVS
  nvs_blob_read(T_IRON, T_timeouts, &t, sizeof(decltype(t)));
  // I do not want work with ms in menu, so let's convert it to sec/min

  _timeout.at(0) = t.standby / 1000;    // sec
  _timeout.at(1) = t.idle / 60000;      // min
  _timeout.at(2) = t.suspend / 60000;   // min
  _timeout.at(3) = t.boost / 1000;      // sec

  _buildMenu();
}

ViSet_TimeoutsSetup::~ViSet_TimeoutsSetup(){
  IronTimeouts t;
  t.standby = _timeout.at(0) * 1000;
  t.idle    = _timeout.at(1) * 60000;
  t.suspend = _timeout.at(2) * 60000;
  t.boost   = _timeout.at(3) * 1000;

  // save settings to NVS
  nvs_blob_write(T_IRON, T_timeouts, &t, sizeof(decltype(t)));
  // send command to reload time settings
  EVT_POST(IRON_SET_EVT, e2int(iron_t::reloadTimeouts));
}

void ViSet_TimeoutsSetup::_buildMenu(){
  // default timeout values
  constexpr std::array<int32_t, 4> def_timeouts_min = {
    TIMEOUT_STANDBY_MIN,
    TIMEOUT_IDLE_MIN,
    TIMEOUT_SUSPEND_MIN,
    TIMEOUT_BOOST_MIN
  };

  constexpr std::array<int32_t, 4> def_timeouts_max = {
    TIMEOUT_STANDBY_MAX,
    TIMEOUT_IDLE_MAX,
    TIMEOUT_SUSPEND_MAX,
    TIMEOUT_BOOST_MAX
  };

  constexpr std::array<int32_t, 4> def_timeouts_step = {
    TIMEOUT_STANDBY_STEP,
    TIMEOUT_IDLE_STEP,
    TIMEOUT_SUSPEND_STEP,
    TIMEOUT_BOOST_STEP
  };

  constexpr std::array<const char*, 4> def_timeouts_label = {
    dictionary[D_sec],
    dictionary[D_min],
    dictionary[D_min],
    dictionary[D_sec]
  };


  // create page "Settings->Timeouts"
  muiItemId root_page = makePage(menu_MainConfiguration.at(1));

  // create "Page title" item, bind it to root page
  addMuippItem(new MuiItem_U8g2_PageTitle(u8g2, nextIndex(), MAIN_MENU_FONT1 ), root_page);

  // create and add to main page a list with settings selection options
  muiItemId scroll_list_id = nextIndex();

  // Timeouts menu dynamic scroll list
  auto list = new MuiItem_U8g2_DynamicScrollList(u8g2, scroll_list_id,
    [](size_t index){ return menu_TimeoutOpts.at(index); },         // this lambda will feed localized strings to the MuiItem list builder class
    [](){ return menu_TimeoutOpts.size(); },
    nullptr,                                                        // action callback
    MAIN_MENU_Y_SHIFT, 3,                                           // offset for each line of text and total number of lines in menu
    MAIN_MENU_X_OFFSET, MAIN_MENU_Y_OFFSET,                         // x,y cursor
    MAIN_MENU_FONT3, MAIN_MENU_FONT3
  );

  // dynamic list will act as page selector
  list->listopts.page_selector = true;
  list->listopts.back_on_last = true;
  list->on_escape = mui_event_t::quitMenu;                          // this is the only active element on a page, so I quit on escape
  // move menu object into MuiPP page
  addMuippItem(list, root_page);
  pageAutoSelect(root_page, scroll_list_id);                        // the only item on page must be autoselected


  // create "Page title" item for sub pages (I will use another font there)
  muiItemId title2_id = nextIndex();
  addMuippItem( std::make_shared< MuiItem_U8g2_PageTitle > (u8g2, title2_id, PAGE_TITLE_FONT) );

  // autogenerate items for each timeout setting value
  for (auto i = 0; i != _timeout.size(); ++i){
    // make page, bound to root page
    muiItemId page = makePage(menu_TimeoutOpts.at(i), root_page);
    // add page title element
    addItemToPage(title2_id, page);
    // create num slider Mui Item
    muiItemId idx = nextIndex();
    auto hslide = new MuiItem_U8g2_NumberHSlide<unsigned>(
      u8g2, idx,
      NULL,   // label
      _timeout.at(i),
      def_timeouts_min[i], def_timeouts_max[i], def_timeouts_step[i],
      numeric_to_string,
      nullptr, nullptr, nullptr,
      NUMERIC_FONT1, MAIN_MENU_FONT2,
      u8g2.getDisplayWidth()/2, u8g2.getDisplayHeight()/2, NUMBERSLIDE_X_OFFSET
    );
    // since this item is the only active on the page, return to prev page on unselect
    hslide->on_escape = mui_event_t::prevPage;
    // add num slider to page
    addMuippItem(hslide, page);
    // autoselect this item
    pageAutoSelect(page, idx);

    // value label hint position in the bottom center of screen
    auto txt = new MuiItem_U8g2_StaticText(u8g2, nextIndex(), def_timeouts_label[i], PAGE_TITLE_FONT, u8g2.getDisplayWidth()/2, u8g2.getDisplayHeight());
    txt->setTextAlignment( text_align_t::center, text_align_t::bottom );
    addMuippItem(txt, page);
  }

  // start menu from root page
  menuStart(root_page);
}



// *****************************
// *** MUI entities


std::string numeric_to_string(int32_t v){
  std::ostringstream oss;
  //oss << std::setprecision(1) << number;
  oss << v;
  return oss.str().data();
}




// *****************************
// *** An instance of HID object
IronHID hid;
