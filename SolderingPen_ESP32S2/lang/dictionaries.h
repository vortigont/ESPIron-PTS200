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
#include "lang_en_us.h"
#include "lang_ru_ru.h"
//#include "lang_zh_cn.h"
//#include "lang_zh_tw.h"

/**
 *  Dictionary with references to text strings
 *  the order of arrays must match ordering lang_index enum
 */
static constexpr std::array< std::array<const char*, D_____SIZE>, L____SIZE> dict = {
    lang_en_us::dictionary,
    lang_ru_ru::dictionary
};

