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

// List of available translations
enum lang_index {
    L_en_us = (0),
    L_ru_ru,
//    L_zh_cn,
//    L_zh_tw,
    L____SIZE
};

/**
 * Text-Dictionary Enums for language resources
 * the order of enums must match with elements in dictionary
 */
enum dict_index {
    D_boost = (0),
    D_error,
    D_heating,
    D_idle,
    D_notip,
    D_set_t,
    D_standby,
    D_____SIZE
};
