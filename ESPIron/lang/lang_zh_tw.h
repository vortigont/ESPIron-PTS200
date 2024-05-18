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
#include "lang_zh_cn.h"

#define MAINSCREEN_FONT     u8g2_font_unifont_t_chinese3

// ZH-TW
namespace lang_zh_tw {
// ZH-TW
static constexpr const char* T_Error = "錯誤";
static constexpr const char* T_set_T = "設溫:";

// ZH-CN
static constexpr std::array<const char *, D_____SIZE> dictionary = {
    lang_zh_cn::T_Boost,
    T_Error,
    lang_zh_cn::T_Idle,
    lang_en_us::T_NoTip,
    T_set_T,
    lang_zh_cn::T_standby
};


} // end  of namespace lang_zh_tw
