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
#define MENU_PWR_CONTROL_CFG_SIZE   4

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
 * the order of enums MUST match with elements in dictionary array
 */
enum dict_index {
    D_Boost = (0),
    D_Error,
    D_Disabled,
    D_Heating,
    D_idle,
    D_min,
    D_none,
    D_NoTip,
    D_OK,
    D_Ramping,
    D_PwrRamp_label,
    D_PwrRamp_hint,
    D_return,
    D_PDVoltage,
    D_QCMode,
    D_QCVoltage,
    D_sec,
    D_SaveLast_box,
    D_SaveLast_hint,
    D_Settings,
    D_set_t,
    D_Standby,
// Notes goes below
    D_Note_QCWarn,
    D_Temp_SaveLastWrkDescr,
    DICT___SIZE
};


// put common international unicode strings here

static constexpr const char* T_CelsiusChar = "°C";
