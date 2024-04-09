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
#include <U8g2lib.h>  // https://github.com/olikraus/u8g2
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


void IronScreen::init(){
  u8g2.initDisplay();
  u8g2.begin();
  u8g2.sendF("ca", 0xa8, 0x3f);
  u8g2.enableUTF8Print();

  _viset = std::make_unique<VisualSetMainScreen>();

/*  TBD
  if(_hand_side){
    u8g2.setDisplayRotation(U8G2_R3);
  }else{
    u8g2.setDisplayRotation(U8G2_R1);
  }
*/

}

//  *********************
//  ***   IronHID     ***

IronHID::IronHID() : _encdr(BUTTON_DECR, BUTTON_INCR, LOW), _btn(BUTTON_ACTION, LOW) {
  _set_button_menu_callbacks();
}

void IronHID::init(const Temperatures& t){
  // initialize screen
  _screen.init();

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

  // get initial working temp.
  _wrk_temp = t.working;

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

}

// encoder events executor, it works in cooperation with _menu object
void IronHID::_encoder_events(esp_event_base_t base, int32_t id, void* event_data){
  switch(_menu.getMenuLevel()){
    // main working mode - encoder controls Iron temperature
    case 0 : {
      _wrk_temp = reinterpret_cast<EventMsg*>(event_data)->cntr;
      EVT_POST_DATA(IRON_SET_EVT, e2int(iron_t::workTemp), &_wrk_temp, sizeof(_wrk_temp));
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
      _encdr.setCounter(_wrk_temp, 5, TEMP_MIN, TEMP_MAX);
      _encdr.setMultiplyFactor(2);
      break;

  }
}

// actions for middle button in main mode 
void IronHID::_menu_0_main_mode(ESPButton::event_t e, const EventMsg* m){
  LOGD(T_HID, printf, "Button Menu lvl:0 e:%d\n", e);
  switch(e){
    // Use click event to toggle iron working mode on/off
    case event_t::click :
      EVT_POST(IRON_SET_EVT, e2int(iron_t::workModeToggle));
      break;

    // use longPress to toggle boost mode
    case event_t::longPress :
      EVT_POST(IRON_SET_EVT, e2int(iron_t::boostModeToggle));
      break;

    // pick multiClicks
    case event_t::multiClick :
      // todo: doubleclick for entering Menu
      //EVT_POST(IRON_SET_EVT, e2int(iron_t::boostModeToggle));
      break;

  }

}


// ***** VisualSet *****
VisualSet::VisualSet() {
  // subscribe to all events on a bus
  ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
                    evt::get_hndlr(),
                    ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID,
                    VisualSet::_event_picker,
                    this, &_evt_handler)
  );

  // loag UI language index
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> nvs = nvs::open_nvs_handle(T_UI, NVS_READONLY, &err);

  if (err == ESP_OK) {
    nvs->get_item(T_lang, lang);
  }
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

VisualSetMainScreen::VisualSetMainScreen() : VisualSet(){
  // request working temperature from IronController
  EVT_POST(IRON_GET_EVT, e2int(iron_t::workTemp));

  // the screen will redraw at first event from sensors
  //_drawMainScreen();
}

void VisualSetMainScreen::_drawMainScreen(){
  u8g2.clearBuffer();

  if(lang == 0){
    u8g2.setFont(u8g2_font_unifont_t_chinese3);
  } else
    u8g2.setFont(PTS200_16);

  u8g2.setFontPosTop();

  // draw status of heater 绘制加热器状态
  u8g2.setCursor(0, 0 + SCREEN_OFFSET);
  switch (_state){
    case ironState_t::idle :
      u8g2.print(dict[lang][D_idle]);
      break;
    case ironState_t::working :
      u8g2.print(dict[lang][D_heating]);
      break;
    case ironState_t::standby :
      u8g2.print(dict[lang][D_standby]);
      break;
    case ironState_t::boost :
      u8g2.print(dict[lang][D_boost]);
      break;
    case ironState_t::notip :
      u8g2.print(dict[lang][D_notip]);
      break;

    default:;
  }

  // print working temperature in the upper right corner
  //    u8g2.drawUTF8(0, 0 + SCREEN_OFFSET, "设温:");
  //u8g2.drawUTF8(OFFSET_TARGET_TEMP_1, 0 + SCREEN_OFFSET, dict[lang][D_set_t]);   // target temperature
  //u8g2.setFont(u8g2_font_unifont_t_chinese3);
  u8g2.setCursor(OFFSET_TARGET_TEMP_1, 0 + SCREEN_OFFSET);
  //u8g2.drawUTF8(OFFSET_TARGET_TEMP_1, 0 + SCREEN_OFFSET, "T:");   // target temperature  
  //u8g2.setCursor(OFFSET_TARGET_TEMP_2, 0 + SCREEN_OFFSET);
  u8g2.print("T:");
  u8g2.print(_wrk_temp, 0);
  u8g2.print("C");

/*
  if(lang != 0){
    u8g2.setFont(PTS200_16);
  }

  if (_tip_temp > TEMP_NOTIP)
    u8g2.print(dict[lang][D_error]);
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

void VisualSetMainScreen::_evt_sensor(int32_t id, void* data){
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

  _drawMainScreen();
}

void VisualSetMainScreen::_evt_notify(int32_t id, void* data){
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

void VisualSetMainScreen::_evt_cmd(int32_t id, void* data){
  switch(static_cast<evt::iron_t>(id)){
    case evt::iron_t::workTemp :
      _wrk_temp = *reinterpret_cast<int32_t*>(data);
    break;
  }
}

void VisualSetMainScreen::_evt_state(int32_t id, void* data){
  switch(static_cast<evt::iron_t>(id)){
    case evt::iron_t::workTemp :
      _wrk_temp = *reinterpret_cast<int32_t*>(data);
    break;
  }
}

// *****************************
// *** An instance of HID object

IronHID hid;