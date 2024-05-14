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
//#include "U8g2lib.h"  // https://github.com/olikraus/u8g2
#include "lang/lang_en_us.h"
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

#define TIMER_DISPLAY_REFRESH     10    // 10 fps max   (screen redraw is ~35 ms)

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
                              [](TimerHandle_t h) { static_cast<IronHID*>(pvTimerGetTimerID(h))->viset->drawScreen(); }
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
  };
}

void IronHID::init(){
  // link our event loop with ESPButton's loop
  ESPButton::set_event_loop_hndlr(evt::get_hndlr());

  // create default ViSet with Main Work screen
  switchViSet(viset_evt_t::vsMainScreen);

  // initialize screen
  _init_screen();

  // enable middle button
  _btn.enable();
  // enable 'encoder' buttons
  _encdr.begin();
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
      //switchScreen(vset_t::configMenu);
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
  // create "Page title" item
  MuiItem_pt p = std::make_shared< MuiItem_U8g2_PageTitle > (u8g2, page_title_id, MAIN_MENU_FONT1 );
  // add item to menu and bind it to "Main Settings" page
  addMuippItem(std::move(p), root_page);

  // bind  "Page title" item with "Settings->Timers" page
  addItemToPage(page_title_id, page_set_time);
  // bind  "Page title" item with "Settings->Tip" page
  addItemToPage(page_title_id, page_set_tip);
  // bind  "Page title" item with "Settings->Power" page
  addItemToPage(page_title_id, page_set_pwr);
  // bind  "Page title" item with "Settings->Info" page
  addItemToPage(page_title_id, page_set_info);

/*
 // checkboxes
  MuiItem_U8g2_CheckBox *box = new MuiItem_U8g2_CheckBox(u8g2, nextIndex(), "Some box", true, nullptr, MAINSCREEN_FONT, 0, 25);
  MuiItem_U8g2_CheckBox *box2 = new MuiItem_U8g2_CheckBox(u8g2, nextIndex(), "Some box2", false, nullptr, MAINSCREEN_FONT, 0, 40);
  addMuippItem(box, root_page);
  addMuippItem(box2, root_page);
*/


  // generate idx for "Back Button" item
  muiItemId bbuton_id = nextIndex();
  // create "Back Button" item
  MuiItem_pt bb = std::make_shared< MuiItem_U8g2_BackButton > (u8g2, bbuton_id, dictionary[D_return], MAIN_MENU_FONT1, PAGE_BACK_BTN_X_OFFSET, PAGE_BACK_BTN_Y_OFFSET);

  // add item to menu and bind it to "Settings->Timers" page
  addMuippItem(std::move(bb), page_set_time);

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

  // set dynamic list will act as page selector
  //menu->listopts.page_selector = true;
  menu->listopts.back_on_last = true;
  menu->on_escape = mui_event_t::quitMenu;                          // quit menu on escape
  // move menu object into MuiPP page
  addMuippItem(menu, root_page);
  // root_page
  pageAutoSelect(root_page, scroll_list_id);

  // start menu from page mainmenu
  menuStart(root_page);
}

void ViSet_MainMenu::_submenu_selector(size_t index){
  LOGD(T_HID, printf, "_submenu_selector:%u\n", index);
  switch (index){
    case 0 :  // temp settings
      EVT_POST(IRON_VISET, e2int(viset_evt_t::vsMenuTemperature));
      break;
    default:
      EVT_POST(IRON_VISET, e2int(viset_evt_t::vsMainScreen));
  }
}


//  **************************************
//  ***   Temperature Control Menu     ***

ViSet_TemperatureSetup::ViSet_TemperatureSetup(GPIOButton<ESPEventPolicy> &button, PseudoRotaryEncoder &encoder) : MuiMenu(button, encoder){
  // go back to prev viset on exit
  parentvs = viset_evt_t::goBack;
  // load temperature values from NVS
  nvs_blob_read(T_IRON, T_temperatures, static_cast<void*>(&_temp), sizeof(Temperatures));

  _buildMenu();
}

ViSet_TemperatureSetup::~ViSet_TemperatureSetup(){
  //LOGD(T_HID, println, "save temp settings");
  // save temp settings to NVS
  nvs_blob_write(T_IRON, T_temperatures, static_cast<void*>(&_temp), sizeof(Temperatures));
  // send command to reload temp settings
  EVT_POST(IRON_SET_EVT, e2int(iron_t::reloadTemp));
}

void ViSet_TemperatureSetup::_buildMenu(){
  // create page "Settings->Temperature"
  muiItemId root_page = makePage(menu_MainConfiguration.at(0));

  // generate idx for "Page title" item
  //muiItemId page_title_menu = nextIndex();
  // create "Page title" item
  MuiItem_pt p = std::make_shared< MuiItem_U8g2_PageTitle > (u8g2, nextIndex(), MAIN_MENU_FONT1 );
  // add item to menu and bind it to "Main Settings" page
  addMuippItem(std::move(p), root_page);

  // create and add to main page a list with settings selection options
  muiItemId scroll_list_id = nextIndex();

  // Temperature menu dynamic scroll list
  MuiItem_U8g2_DynamicScrollList *list = new MuiItem_U8g2_DynamicScrollList(u8g2, scroll_list_id,
    [](size_t index){ /* Serial.printf("idx:%u\n", index); */ return menu_TemperatureOpts.at(index); },   // this lambda will feed localized strings to the MuiItem list builder class
    [](){ return menu_TemperatureOpts.size(); },
    nullptr,                                                        // action callback
    MAIN_MENU_Y_SHIFT, 3,                                           // offset for each line of text and total number of lines in menu
    MAIN_MENU_X_OFFSET, MAIN_MENU_Y_OFFSET,                         // x,y cursor
    MAIN_MENU_FONT3, MAIN_MENU_FONT3
  );

  // dynamic list will act as page selector
  list->listopts.page_selector = true;
  list->listopts.back_on_last = true;
  list->on_escape = mui_event_t::prevPage;                          // go to previous page on escape
  // move menu object into MuiPP page
  addMuippItem(list, root_page);
  pageAutoSelect(root_page, scroll_list_id);

  // ***
  // page "Save Work temp"
  muiItemId page = makePage(menu_TemperatureOpts.at(0), root_page);
  muiItemId title2_id = nextIndex();
  // create "Page title" item
  MuiItem_pt title2 = std::make_shared< MuiItem_U8g2_PageTitle > (u8g2, title2_id, PAGE_TITLE_FONT);
  // add item to menu and bind it to "save work temp" page
  addMuippItem(std::move(title2), page);

  // save last work temp checkbox
  MuiItem_U8g2_CheckBox *box = new MuiItem_U8g2_CheckBox(u8g2, nextIndex(), dictionary[D_SaveLast_box], _temp.savewrk, [this](size_t v){ _temp.savewrk = v; }, MAINSCREEN_FONT, 0, 25);
  addMuippItem(box, page);

  // text hint
  MuiItem_U8g2_StaticText *txt = new MuiItem_U8g2_StaticText(u8g2, nextIndex(), dictionary[D_SaveLast_hint], MAIN_MENU_FONT1, 0, 25);
  addMuippItem(txt, page);

  // generate idx for "Back Button" item
  muiItemId bbuton_id = nextIndex();
  // create "Back Button" item
  MuiItem_pt bb = std::make_shared< MuiItem_U8g2_BackButton > (u8g2, bbuton_id, dictionary[D_return], MAIN_MENU_FONT1, PAGE_BACK_BTN_X_OFFSET, PAGE_BACK_BTN_Y_OFFSET);

  // add item and bind it to "save work temp" page
  addMuippItem(std::move(bb), page);

  // ***
  // page "Default temp"
  page = makePage(menu_TemperatureOpts.at(1), root_page);
  // page title element
  addItemToPage(title2_id, page);
  muiItemId idx = nextIndex();
  // create num slider
  MuiItem_U8g2_NumberHSlide<int32_t> *hslide = new MuiItem_U8g2_NumberHSlide<int32_t>(
    u8g2, idx,
    NULL,
    _temp.deflt,
    TEMP_MIN, TEMP_MAX, TEMP_STEP,
    int_to_string,
    nullptr, nullptr, nullptr,
    NUMERIC_FONT1, MAIN_MENU_FONT2,
    u8g2.getDisplayWidth()/2, u8g2.getDisplayHeight()/2, 10
  );
  hslide->on_escape = mui_event_t::prevPage; 
  // add num slider
  addMuippItem(hslide, page);
  pageAutoSelect(page, idx);


  // ***
  // page "Standby temp"
  page = makePage(menu_TemperatureOpts.at(2), root_page);
  // page title element
  addItemToPage(title2_id, page);
  idx = nextIndex();
  // create num slider
  hslide = new MuiItem_U8g2_NumberHSlide<int32_t>(
    u8g2, idx,
    NULL,
    _temp.standby,
    TEMP_STANDBY_MIN, TEMP_STANDBY_MAX, TEMP_STANDBY_STEP,
    int_to_string,
    nullptr, nullptr, nullptr,
    NUMERIC_FONT1, MAIN_MENU_FONT2,
    u8g2.getDisplayWidth()/2, u8g2.getDisplayHeight()/2, 10
  );
  hslide->on_escape = mui_event_t::prevPage; 
  // add num slider
  addMuippItem(hslide, page);
  pageAutoSelect(page, idx);
  // back button
  addItemToPage(bbuton_id, page);


  // ***
  // page "Boost temp"
  page = makePage(menu_TemperatureOpts.at(3), root_page);
  // page title element
  addItemToPage(title2_id, page);
  idx = nextIndex();
  // create num slider
  hslide = new MuiItem_U8g2_NumberHSlide<int32_t>(
    u8g2, idx,
    NULL,
    _temp.boost,
    TEMP_BOOST_MIN, TEMP_BOOST_MAX, TEMP_BOOST_STEP,
    int_to_string,
    nullptr, nullptr, nullptr,
    NUMERIC_FONT1, MAIN_MENU_FONT2,
    u8g2.getDisplayWidth()/2, u8g2.getDisplayHeight()/2, 10
  );
  hslide->on_escape = mui_event_t::prevPage; 
  // add num slider
  addMuippItem(hslide, page);
  pageAutoSelect(page, idx);

  // start menu from root page
  menuStart(root_page);
}






// *****************************
// *** MUI entities

std::string int_to_string(int32_t v){
  std::ostringstream oss;
  //oss << std::setprecision(1) << number;
  oss << v;
  return oss.str().data();
}




// *****************************
// *** An instance of HID object

IronHID hid;
