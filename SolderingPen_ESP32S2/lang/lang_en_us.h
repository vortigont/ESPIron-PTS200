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

#define MAINSCREEN_FONT     u8g2_font_unifont_t_cyrillic
// 12x16 wrong? small gothic font
#define MAIN_MENU_FONT1     u8g2_font_glasstown_nbp_t_all
// 10x14 OK!
#define MAIN_MENU_FONT2     u8g2_font_smart_patrol_nbp_tr
// 10x14 OK!
#define MAIN_MENU_FONT3     u8g2_font_unifont_t_cyrillic

// 12x17
//#define MAIN_MENU_FONT3     u8g2_font_shylock_nbp_t_all
// 7x7
//#define MAIN_MENU_FONT3     u8g2_font_mercutio_sc_nbp_t_all

#define MAIN_MENU_X_OFFSET  10
#define MAIN_MENU_Y_OFFSET  12
#define MAIN_MENU_Y_SHIFT   15

#define PAGE_BACK_BTN_X_OFFSET  100
#define PAGE_BACK_BTN_Y_OFFSET  50

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
static constexpr const char* T_return = "<Back";            // return back in menu's

// Temp settings descr
static constexpr const char* T_Temp_SaveLastWrkDescr = "Save last work Temperature";

// Menu Page names
static constexpr const char* T_Settings = "Settings";

// Main configuration menu
static constexpr const char* T_TempSettings = "Temperature";
static constexpr const char* T_TimersSettings = "Timeouts";
static constexpr const char* T_TipSettings = "Tip";
static constexpr const char* T_PowerSupply = "Power Supply";
static constexpr const char* T_Information = "Information";
static constexpr const char* T_Language = "Language";

// Temperature settings menu
static constexpr const char* T_SaveLastT = "Save work temp.";
static constexpr const char* T_TempDefltWrk = "Default temp.";
static constexpr const char* T_TempSleep = "Standby temp.";
static constexpr const char* T_TempBoost = "Boost step T";
// Temperature settings hints
static constexpr const char* T_SaveLast_box = "use last temp.";
static constexpr const char* T_SaveLast_hint = "instead of default one";



} // end  of namespace lang_en_us

/**
 * @brief general purpose dictionary
 * order of items MUST match enum dict_index
 * 
 */
static constexpr std::array<const char *, DICT___SIZE> dictionary = {
    lang_en_us::T_Boost,
    lang_en_us::T_Error,
    lang_en_us::T_Heating,
    lang_en_us::T_Idle,
    lang_en_us::T_NoTip,
    lang_en_us::T_return,
    lang_en_us::T_SaveLast_box,
    lang_en_us::T_SaveLast_hint,
    lang_en_us::T_Settings,
    lang_en_us::T_setT,
    lang_en_us::T_standby,
    lang_en_us::T_Temp_SaveLastWrkDescr
};

// Main Configuration menu items
static constexpr std::array<const char *, MENU_MAIN_CFG_SIZE> menu_MainConfiguration = {
    lang_en_us::T_TempSettings,
    lang_en_us::T_TimersSettings,
    lang_en_us::T_TipSettings,
    lang_en_us::T_PowerSupply,
    lang_en_us::T_Information,
    lang_en_us::T_return
};

// Main Configuration menu items
static constexpr std::array<const char *, MENU_TEMPERATURE_CFG_SIZE> menu_TemperatureOpts = {
    lang_en_us::T_SaveLastT,
    lang_en_us::T_TempDefltWrk,
    lang_en_us::T_TempSleep,
    lang_en_us::T_TempBoost,
    lang_en_us::T_return
};
