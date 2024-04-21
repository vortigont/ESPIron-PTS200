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
#include <array>
#include "i18n.h"

// 12x16 wrong? small gothic font
#define MAIN_MENU_FONT1     u8g2_font_glasstown_nbp_t_all
// 10x14 OK!
#define MAIN_MENU_FONT2     u8g2_font_smart_patrol_nbp_tr
// 12x17
#define MAIN_MENU_FONT3     u8g2_font_shylock_nbp_t_all
// 7x7
//#define MAIN_MENU_FONT3     u8g2_font_mercutio_sc_nbp_t_all

#define MAIN_MENU_X_OFFSET  10
#define MAIN_MENU_Y_OFFSET  12
#define MAIN_MENU_Y_SHIFT   15


// EN-US
namespace lang_en_us {

// EN US
static constexpr const char* T_Boost = "Boost";
static constexpr const char* T_Error = "Error";
static constexpr const char* T_Heating = "Heating";
static constexpr const char* T_Idle = "Idle";
static constexpr const char* T_NoTip = "No tip!";           // state display when tip is missing
static constexpr const char* T_setT = "Set:";               // Target temperature on main screen
static constexpr const char* T_standby = "Standby";         // state display in 'Standby'
static constexpr const char* T_return = "<Return";           // return back in menu's

// Menu Page names
static constexpr const char* T_Settings = "Settings";

// Main configuration menu
static constexpr const char* T_TipSettings = "Tip Настройки";
static constexpr const char* T_TempSettings = "Temp Настройки";
static constexpr const char* T_TimersSettings = "Timers Настройки";
static constexpr const char* T_Information = "Информация";
static constexpr const char* T_Language = "Language";

} // end  of namespace lang_en_us

// general purpose dictionary
static constexpr std::array<const char *, D_____SIZE> dictionary = {
    lang_en_us::T_Boost,
    lang_en_us::T_Error,
    lang_en_us::T_Heating,
    lang_en_us::T_Idle,
    lang_en_us::T_NoTip,
    lang_en_us::T_Settings,
    lang_en_us::T_setT,
    lang_en_us::T_standby
};

// Main Configuration menu items
static constexpr std::array<const char *, MENU_MAIN_CFG_SIZE> menu_MainConfiguration = {
    lang_en_us::T_TempSettings,
    lang_en_us::T_TimersSettings,
    lang_en_us::T_TipSettings,
    lang_en_us::T_Information,
    lang_en_us::T_Language,
    lang_en_us::T_standby,
    lang_en_us::T_Heating,
    lang_en_us::T_return
};
