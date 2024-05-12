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
#include "lang_en_us.h"

#define MAINSCREEN_FONT     u8g2_font_unifont_t_chinese3


// ZH-CN
namespace lang_zh_cn {
// ZH-CN
static constexpr const char* T_Boost = "提温";
static constexpr const char* T_Error = "错误";
static constexpr const char* T_Idle = "禁用";       // deactivated
static constexpr const char* T_set_T = "设温:";
static constexpr const char* T_standby = "休眠";

// ZH-CN
static constexpr std::array<const char *, D_____SIZE> dictionary = {
    T_Boost,
    T_Error,
    T_Idle,
    lang_en_us::T_NoTip,
    T_set_T,
    T_standby
};


} // end  of namespace lang_zh_cn
