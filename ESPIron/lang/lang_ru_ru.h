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

// 10x16 contains lat/cyr chars
#define MAIN_MENU_FONT3     u8g2_font_mercutio_sc_nbp_t_all

// 12x16
// u8g2_font_glasstown_nbp_t_all

// 10x15 https://github.com/olikraus/u8g2/wiki/fntgrpnbp
//u8g2_font_nine_by_five_nbp_t_all

// 11x17 https://github.com/olikraus/u8g2/wiki/fntgrpnbp
//u8g2_font_guildenstern_nbp_t_all

// RU-RU
namespace lang_ru_ru {

static constexpr const char* T_Boost = "Разгон";
static constexpr const char* T_Error = "Ошибка";
static constexpr const char* T_Heating = "Нагрев";
static constexpr const char* T_Idle = "Отключен";
static constexpr const char* T_NoTip = "Нет жала!";           // state display when tip is missing
static constexpr const char* T_setT = "Уст:";
static constexpr const char* T_standby = "Ожидание";          // state display in 'Standby'
static constexpr const char* T_return = "<Выход";            // return back in menu's

// Main configuration menu
static constexpr const char* T_TipSettings = MUI_10 "Настройки жал";
static constexpr const char* T_TempSettings = MUI_20 "Настройки температуры";
static constexpr const char* T_TimersSettings = MUI_30 "Настройки таймеров";
static constexpr const char* T_Information = MUI_40 "Информация";

// general purpose dictionary
static constexpr std::array<const char *, D_____SIZE> dictionary = {
    T_Boost,
    T_Error,
    T_Heating,
    T_Idle,
    T_NoTip,
    T_setT,
    T_standby
};

// Main Configuration menu items
static constexpr std::array<const char *, MENU_MAIN_CFG_SIZE> menu_MainConfiguration = {
    T_TempSettings,
    T_TimersSettings,
    T_TipSettings,
    T_Information,
    lang_en_us::T_Language,
    T_return
};



} // end  of namespace lang_ru_ru

