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
#if defined CUSTOM_CFG
# include CUSTOM_CFG
#warning "include custom cfg file"
#else
#include "config.h"
#endif
#include <stdint.h>

enum class ironState_t {
    idle = 0,     // everything is on, exept heater, Iron is in idle state, i.e. it was just powered ON
    working,      // everything is on, full function
    standby,      // heater is in standby mode, i.e. keeping warm after a certain period of inactivity
    suspend,      // hibernation mode
    boost,        // heater temperature is increased for a short period of time
    setup,        // iron is configuration mode, i.e. working with screen menu, heater switches off
    notip         // tip is missing or failed

};

// working temperature values
struct Temperatures {
    int32_t working{TEMP_DEFAULT}, standby{TEMP_SLEEP}, boost{TEMP_BOOST}, deflt{TEMP_DEFAULT};
    bool savewrk{false};
};

