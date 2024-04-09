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

// EN-US
namespace lang_en_us {

// EN US
static constexpr const char* T_Boost = "Boost";
static constexpr const char* T_Error = "Error";
static constexpr const char* T_Heating = "Heating";
static constexpr const char* T_Idle = "Idle";
static constexpr const char* T_NoTip = "No tip!";       // state display when tip is missing
static constexpr const char* T_setT = "Set:";         // Target temperature on main screen
static constexpr const char* T_standby = "Standby";     // state display in 'Standby'

static constexpr std::array<const char *, D_____SIZE> dictionary = {
    T_Boost,
    T_Error,
    T_Heating,
    T_Idle,
    T_NoTip,
    T_setT,
    T_standby
};

} // end  of namespace lang_en_us

