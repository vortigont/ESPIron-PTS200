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


// shortcut alias
using ESPButton::event_t;
using evt::iron_t;


void IronHID::_init_screen(){
#ifndef NO_DISPLAY
  u8g2.initDisplay();
  u8g2.begin();
  u8g2.sendF("ca", 0xa8, 0x3f);
  u8g2.enableUTF8Print();
#endif

  // switch to default VisualSet
  switchScreen();

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

void IronHID::switchScreen(vset_t v){
  // todo: do I need a locking mutex here?
  LOGI(T_HID, printf, "switch screen to:%d\n", e2int(v));
  switch(v){
    case vset_t::mainScreen :
      viset = std::make_unique<ViSet_MainScreen>();
      break;
    case vset_t::configMenu :
      viset = std::make_unique<ViSet_ConfigurationMenu>();
      break;

  };
}


//  *********************
//  ***   IronHID     ***

IronHID::IronHID() : _encdr(BUTTON_DECR, BUTTON_INCR, LOW), _btn(BUTTON_ACTION, LOW) {
  _set_button_menu_callbacks();
}

void IronHID::init(const Temperatures& t){
  // initialize screen
  _init_screen();

  // link our event loop with ESPButton
  ESPButton::set_event_loop_hndlr(evt::get_hndlr());

  // subscribe to button events
  if (!_btn_evt_handler){
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
                      evt::get_hndlr(),
                      EBTN_EVENTS, ESP_EVENT_ANY_ID,
                      [](void* self, esp_event_base_t base, int32_t id, void* data) {
                          // pick action button events and pass it to _menu member
                          LOGD(T_HID, printf, "btn event:%d, gpio:%d\n", id, reinterpret_cast<EventMsg*>(data)->gpio);
                          if (reinterpret_cast<EventMsg*>(data)->gpio == BUTTON_ACTION)
                            static_cast<IronHID*>(self)->_menu.handleEvent(ESPButton::int2event_t(id), reinterpret_cast<EventMsg*>(data));
                        },
                      this, &_btn_evt_handler)
    );
  }

  if (!_enc_evt_handler){
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
                      evt::get_hndlr(),
                      EBTN_ENC_EVENTS, ESP_EVENT_ANY_ID,
                      [](void* self, esp_event_base_t base, int32_t id, void* data) {
                          LOGD(T_HID, printf, "enc event:%d, cnt:%d\n", id, reinterpret_cast<EventMsg*>(data)->cntr);
                          // pick encoder events and pass it to _menu member
                          static_cast<IronHID*>(self)->_encoder_events(base, id, data);
                        },
                      this, &_enc_evt_handler)
    );
  }

  // get initial temperatures
  _temp = t;

  // enable middle button
  _btn.enable();
  // enable 'encoder' buttons
  _encdr.begin();
  // set level 0 for button and encoder buttons
  _switch_buttons_modes(0);
}

void IronHID::_set_button_menu_callbacks(){
  // actions for middle button in main mode
  _menu.assign(BUTTON_ACTION, 0, [this](event_t e, const EventMsg* m){ _menu_0_main_mode(e, m); });
  _menu.assign(BUTTON_ACTION, 1, [this](event_t e, const EventMsg* m){ _menu_1_config_navigation(e, m); });

}

// encoder events executor, it works in cooperation with _menu object
void IronHID::_encoder_events(esp_event_base_t base, int32_t id, void* event_data){
  LOGD(T_HID, printf, "_encoder_events:%d, cnt:%d\n", id, reinterpret_cast<EventMsg*>(event_data)->cntr);
  switch(_menu.getMenuLevel()){

    // main Iron screen mode - encoder controls Iron working temperature
    case 0 : {
      _temp.working = reinterpret_cast<EventMsg*>(event_data)->cntr;
      EVT_POST_DATA(IRON_SET_EVT, e2int(iron_t::workTemp), &_temp.working, sizeof(_temp.working));
      break;
    }

    // configuration Menu - encoder sends commands to MuiPP sink
    case 1 : {
      // we do not need counter value here (for now), just figure out if it was increment or decrement via gpio which triggered and event
      if (reinterpret_cast<EventMsg*>(event_data)->gpio == BUTTON_INCR){
        viset->muipp_event( {mui_event_t::moveDown, 0, nullptr} );
      } else {
        viset->muipp_event( {mui_event_t::moveUp, 0, nullptr} );
      }
      break;
    }
  }
}

void IronHID::_switch_buttons_modes(uint32_t level){
  switch(level){
    // main working mode
    case 0 :
      _btn.deactivateAll();
      _btn.enableEvent(event_t::click);
      _btn.enableEvent(event_t::longPress);
      _btn.enableEvent(event_t::multiClick);
      // set encoder to control working temperature
      _encdr.reset();
      _encdr.setCounter(_temp.working, 5, TEMP_MIN, TEMP_MAX);
      _encdr.setMultiplyFactor(2);
      break;

    // config menu navigation
    case 1 :
      _btn.deactivateAll();
      _btn.enableEvent(event_t::click);
      // set encoder to control working temperature
      _encdr.reset();
      //_encdr.setCounter(_temp.working, 5, TEMP_MIN, TEMP_MAX);
      //_encdr.setMultiplyFactor(2);
      break;

  }
}

// actions for middle button when iron is main working mode
void IronHID::_menu_0_main_mode(ESPButton::event_t e, const EventMsg* m){
  LOGD(T_HID, printf, "Button Menu lvl:0 e:%d\n", e);
  switch(e){
    // Use click event to toggle iron working mode on/off
    case event_t::click :
      EVT_POST(IRON_SET_EVT, e2int(iron_t::workModeToggle));
      break;

    // use longPress to enter configuration menu
    case event_t::longPress :
      _menu.setMenuLevel(1);      // switch button menu to mainConfig mode
      _switch_buttons_modes(1);   // switch button events to navigate in menu
      switchScreen(vset_t::configMenu);
      break;

    // pick multiClicks
    case event_t::multiClick :
      // doubleclick to toggle boost mode
      if (m->cntr == 2)
        EVT_POST(IRON_SET_EVT, e2int(iron_t::boostModeToggle));
      break;

  }
}

// actions for middle button in main Configuration menu
void IronHID::_menu_1_config_navigation(ESPButton::event_t e, const EventMsg* m){
  LOGD(T_HID, printf, "Button Menu lvl:1 e:%d\n", e);
  switch(e){
    // Use click event to send Select to MUI menu
    case event_t::click :
      viset->muipp_event( mui_event(mui_event_t::enter) );
      break;

    // use longPress to escape current item
    case event_t::longPress :
      viset->muipp_event( mui_event(mui_event_t::escape) );
      break;
  }
}



// ***** VisualSet - Main Screen *****
VisualSet::VisualSet() {
  // subscribe to all events on a bus
  ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
                    evt::get_hndlr(),
                    ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID,
                    VisualSet::_event_picker,
                    this, &_evt_handler)
  );
}

VisualSet::~VisualSet(){
  // unsubscribe from an event bus
  if (_evt_handler){
    esp_event_handler_instance_unregister_with(evt::get_hndlr(), ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, _evt_handler);
    _evt_handler = nullptr;
  }
}

void VisualSet::_event_picker(void* arg, esp_event_base_t base, int32_t id, void* event_data){
  //LOGV(printf, "VisualSet::_event_picker %s:%d\n", base, id);
  if (base == SENSOR_DATA)
    return reinterpret_cast<VisualSet*>(arg)->_evt_sensor(id, event_data);

  if (base == IRON_NOTIFY)
    return reinterpret_cast<VisualSet*>(arg)->_evt_notify(id, event_data);

  if (base == IRON_SET_EVT)
    return reinterpret_cast<VisualSet*>(arg)->_evt_cmd(id, event_data);

  if (base == IRON_STATE)
    return reinterpret_cast<VisualSet*>(arg)->_evt_state(id, event_data);
}

ViSet_MainScreen::ViSet_MainScreen() : VisualSet(){
  // request working temperature from IronController
  EVT_POST(IRON_GET_EVT, e2int(iron_t::workTemp));

  // the screen will redraw at first event from sensors
  //_drawMainScreen();
}

void ViSet_MainScreen::drawScreen(){
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_unifont_t_chinese3);
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
  u8g2.setCursor(15, 20);
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

    default:
      return;
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
    break;
  }
}

void ViSet_MainScreen::_evt_state(int32_t id, void* data){
  switch(static_cast<evt::iron_t>(id)){
    case evt::iron_t::workTemp :
      _temp.working = *reinterpret_cast<int32_t*>(data);
    break;
  }
}


// ***** VisualSet - ConfigurationMenu *****

ViSet_ConfigurationMenu::ViSet_ConfigurationMenu(){
  _build_menu();
};

void ViSet_ConfigurationMenu::_build_menu(){
  LOGD(T_HID, println, "Build menu");

  // create page with Settings options list
  muiItemId page_mainmenu = makePage(dictionary[D_Settings]);

  // create page "Settings->Temperature"
  muiItemId page_set_temp = makePage(menu_MainConfiguration.at(0), page_mainmenu);

  // create page "Settings->Timers"
  muiItemId page_set_time = makePage(menu_MainConfiguration.at(1), page_mainmenu);

  // create page "Settings->Tip"
  muiItemId page_set_tip = makePage(menu_MainConfiguration.at(2), page_mainmenu);

  // create page "Settings->Power"
  muiItemId page_set_pwr = makePage(menu_MainConfiguration.at(3), page_mainmenu);

  // create page "Settings->Info"
  muiItemId page_set_info = makePage(menu_MainConfiguration.at(4), page_mainmenu);


  // #define MAKE_ITEM(ITEM_TYPE, ARG, PAGE) { std::unique_ptr<MuiItem> p = std::make_unique< ITEM_TYPE > ARG; addMuippItem(std::move(p), PAGE); }
  // MAKE_ITEM( MuiItem_U8g2_PageTitle, (u8g2, nextIndex(), MAIN_MENU_FONT1 ), page_mainmenu);    // u8g2_font_unifont_t_chinese3

  // generate idx for "Page title" item
  muiItemId page_title_id = nextIndex();
  // create "Page title" item
  MuiItem_pt p = std::make_shared< MuiItem_U8g2_PageTitle > (u8g2, page_title_id, MAIN_MENU_FONT1 );
  // add item to menu and bind it to "Main Settings" page
  addMuippItem(std::move(p), page_mainmenu);

  // bind  "Page title" item with "Settings->Temperature" page
  addItemToPage(page_title_id, page_set_temp);
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
  // create "Back Button" item
  MuiItem_pt bb = std::make_shared< MuiItem_U8g2_BackButton > (u8g2, bbuton_id, dictionary[D_return], MAIN_MENU_FONT1, PAGE_BACK_BTN_X_OFFSET, PAGE_BACK_BTN_Y_OFFSET);

  // add item to menu and bind it to "Settings->Temperature" page
  addMuippItem(std::move(bb), page_set_temp);

  // bind  "Back Button" item with "Settings->Timers" page
  addItemToPage(bbuton_id, page_set_time);
  // bind  "Back Button" item with "Settings->Tip" page
  addItemToPage(bbuton_id, page_set_tip);
  // bind  "Back Button" item with "Settings->Power" page
  addItemToPage(bbuton_id, page_set_pwr);
  // bind  "Back Button" item with "Settings->Info" page
  addItemToPage(bbuton_id, page_set_info);

  //std::unique_ptr<MuiItem> p = std::make_unique<MuiItem_U8g2_PageTitle>(u8g2, muipp.nextIndex(), u8g2_font_unifont_t_chinese3);
  //muipp.addMuippItem(std::move(p), cfg_page);

  //MAKE_ITEM( MuiItem_U8g2_PageTitle, (u8g2, muipp.nextIndex(), MAIN_MENU_FONT2, 10, 15), cfg_page);
  //std::unique_ptr<MuiItem> p = std::make_unique<MuiItem_U8g2_PageTitle>(u8g2, muipp.nextIndex(), nullptr, 15, 15);
  //muipp.addMuippItem(std::move(p), cfg_page);


  //MAKE_ITEM( MuiItem_U8g2_PageTitle, (u8g2, muipp.nextIndex(), MAIN_MENU_FONT3, 10, 30), cfg_page);
  //std::unique_ptr<MuiItem> p = std::make_unique<MuiItem_U8g2_PageTitle>(u8g2, muipp.nextIndex(), nullptr, 30, 50);
  //muipp.addMuippItem(std::move(p), cfg_page);

  // create and add to main page a list with settings selection options
  muiItemId menu_scroll_item = nextIndex();
/*
  std::unique_ptr<MuiItem> p = std::make_unique<MuiItem_U8g2_DynamicScrollList>(u8g2, menu_scroll_item,
    [](size_t index){ Serial.printf("idx:%u\n", index); return menu_MainConfiguration.at(index); },   // this lambda will feed localized strings to the MuiItem list builder class
    menu_MainConfiguration.size(),
    MAIN_MENU_Y_SHIFT, 3,                                           // offset for each line of text and total number of lines in menu
    MAIN_MENU_FONT2, MAIN_MENU_FONT3, MAIN_MENU_X_OFFSET, MAIN_MENU_Y_OFFSET
  );
*/
  MuiItem_U8g2_DynamicScrollList *menu = new MuiItem_U8g2_DynamicScrollList(u8g2, menu_scroll_item,
    [](size_t index){ /* Serial.printf("idx:%u\n", index); */ return menu_MainConfiguration.at(index); },   // this lambda will feed localized strings to the MuiItem list builder class
    [](){ return menu_MainConfiguration.size(); },
    nullptr,                                                        // action callback
    MAIN_MENU_Y_SHIFT, 3,                                           // offset for each line of text and total number of lines in menu
    MAIN_MENU_X_OFFSET, MAIN_MENU_Y_OFFSET,                         // x,y cursor
    MAIN_MENU_FONT2, MAIN_MENU_FONT3
  );

  // set dynamic list will act as page selector
  menu->listopts.page_selector = true;
  // move menu object into MuiPP page
  addMuippItem(menu, page_mainmenu);
  // page_mainmenu
  pageAutoSelect(page_mainmenu, menu_scroll_item);

  menuStart(page_mainmenu, menu_scroll_item);
}


void ViSet_ConfigurationMenu::drawScreen(){
  if (!refresh_req) return;

  Serial.printf("st:%lu\n", millis());
  u8g2.clearBuffer();
  render();
  u8g2.sendBuffer();
  Serial.printf("en:%lu\n", millis());

  refresh_req = false;
}

mui_event ViSet_ConfigurationMenu::muipp_event(mui_event e) {
  //LOGD(T_HID, printf, "Got event for muipp lvl:1 e:%u\n", e.eid);

  // forward those messages to muipp
  auto reply = muiEvent(e);
  // any event will trigger screen refresh
  refresh_req = true;
  return reply;
}


// *****************************
// *** MUI entities




// *****************************
// *** An instance of HID object

IronHID hid;
