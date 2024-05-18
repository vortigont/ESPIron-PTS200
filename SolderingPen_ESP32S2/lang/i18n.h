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
#include "MUIU8g2.h"


#define MENU_MAIN_CFG_SIZE          6
#define MENU_TEMPERATURE_CFG_SIZE   5
#define MENU_TIMEOUTS_CFG_SIZE      5

/*
// List of available translations
enum lang_index : uint32_t {
    L_en_us = (0),
    L_ru_ru,
//    L_zh_cn,
//    L_zh_tw,
    L____SIZE
};
*/

/**
 * Text-Dictionary Enums for language resources
 * the order of enums must match with elements in dictionary array
 */
enum dict_index {
    D_boost = (0),
    D_error,
    D_heating,
    D_idle,
    D_min,
    D_notip,
    D_return,
    D_sec,
    D_SaveLast_box,
    D_SaveLast_hint,
    D_Settings,
    D_set_t,
    D_standby,
    D_Temp_SaveLastWrkDescr,
    DICT___SIZE
};


// put common international unicode strings here

static constexpr const char* T_CelsiusChar = "°C";
