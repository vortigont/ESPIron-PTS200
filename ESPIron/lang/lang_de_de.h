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

#define MAINSCREEN_FONT             u8g2_font_unifont_t_cyrillic

// 12x16 wrong? small gothic font
#define SMALL_TEXT_FONT             u8g2_font_glasstown_nbp_t_all
#define SMALL_TEXT_FONT_Y_OFFSET    12

// 12x16 wrong? small gothic font
#define MAIN_MENU_FONT1             u8g2_font_glasstown_nbp_t_all

// 10x14
#define MAIN_MENU_FONT2             u8g2_font_bauhaus2015_tr    //  u8g2_font_smart_patrol_nbp_tr

// 10x14 OK! thin, quite readable
#define MAIN_MENU_FONT3             u8g2_font_unifont_t_cyrillic

// 11x15    nice bold font
#define PAGE_TITLE_FONT             u8g2_font_bauhaus2015_tr
#define PAGE_TITLE_FONT_Y_OFFSET    15

// 12x16 wrong? small gothic font
#define PAGE_TITLE_FONT_SMALL       u8g2_font_glasstown_nbp_t_all

// large numeric font for digits sliders
#define NUMERIC_FONT1               u8g2_font_profont29_tn

// 12x17
//#define MAIN_MENU_FONT3     u8g2_font_shylock_nbp_t_all
// 7x7
//#define MAIN_MENU_FONT3     u8g2_font_mercutio_sc_nbp_t_all

#define MAIN_MENU_X_OFFSET  10
#define MAIN_MENU_Y_OFFSET  15
#define MAIN_MENU_Y_SHIFT   16
#define MAIN_MENU_ROWS      3
#define MENU_LIST_Y_OFFSET  25

#define PAGE_BACK_BTN_X_OFFSET  100
#define PAGE_BACK_BTN_Y_OFFSET  55

#define NUMBERSLIDE_X_OFFSET    10

#define PWR_PD_VALUE_OFFSET     90

// DE-DE
namespace lang_de_de {

// Iron Modes (On main screen)
static constexpr const char* T_Idle         = "Leerlauf";               // state display
static constexpr const char* T_Heating      = "Heizen";                // state display
static constexpr const char* T_Standby      = "Standby";               // state display in 'Standby'
static constexpr const char* T_Boost        = "Boost";                 // state display
static constexpr const char* T_Ramping      = "Hochfahren!";           // state display power ramp
static constexpr const char* T_NoTip        = "Kein Aufsatz!";         // state display when tip is missing

// Others
static constexpr const char* T_Disabled     = "Deaktiviert";           // disabled option
static constexpr const char* T_Error        = "Fehler";
static constexpr const char* T_min          = "Min.";                  // short for 'minutes'
static constexpr const char* T_none         = "Keine";                 // none option
static constexpr const char* T_OK           = "OK";                    // OK label/message
static constexpr const char* T_PDVoltage    = "PD Spannung:";          // PowerDelivery trigger voltage
static constexpr const char* T_QCMode       = "QC Modus:";             // QC trigger mode
static constexpr const char* T_QCVoltage    = "QC Spannung:";          // QC trigger voltage
static constexpr const char* T_sec          = "Sek.";                  // short for 'seconds'
static constexpr const char* T_setT         = "Einstellen:";           // Target temperature on main screen
static constexpr const char* T_return       = "<Zurück";               // return back in menu's

// Menu Page names
static constexpr const char* T_Settings = "Einstellungen";

// Main configuration menu
static constexpr const char* T_TempSettings = "Temperatur";
static constexpr const char* T_TimersSettings = "Zeiteinstellungen";
static constexpr const char* T_TipSettings = "Aufsatz";
static constexpr const char* T_PowerSupply = "Stromversorgung";
static constexpr const char* T_Information = "Information";
static constexpr const char* T_Language = "Sprache";

// Temperature settings menu
static constexpr const char* T_SaveLastT = "Letzte Temp. speichern";
static constexpr const char* T_TempDefltWrk = "Arbeitstemp.";
static constexpr const char* T_TempStandby = "Standby-Temp.";
static constexpr const char* T_TempBoost = "Boost-Temp.";
// Temperature settings hints
static constexpr const char* T_SaveLast_box = "Letzte Temp. verwenden";
static constexpr const char* T_SaveLast_hint = "anstatt der Standardtemperatur";

// Timeouts settings menu
static constexpr const char* T_TimeStandby = "Standby-Zeit";
static constexpr const char* T_TimeIdle = "Leerlaufzeit";
static constexpr const char* T_TimeSuspend = "Pause-Zeit";
static constexpr const char* T_TimeBoost = "Boost-Zeit";

// Power Supply settings menu
static constexpr const char* T_PwrPD = "PD Auslösung";
static constexpr const char* T_PwrQC = "QC Auslösung";
static constexpr const char* T_PwrRamp = "Leistungsrampe";

//  **** Descriptions and Notes ****

// Temp settings descr
static constexpr const char* T_Temp_SaveLastWrkDescr = "Letzte Arbeitstemp. speichern";
// QC feature warning
static constexpr const char* T_Note_QCWarn = "QC Auslösung ist experimentell!!!\nKann instabil sein! Stecker ziehen und erneut einstecken bei QC-Modus-Wechsel!";
// PWM Ramping settings hints
static constexpr const char* T_PwrRamp_label = "PWM Rampe nutzen";
static constexpr const char* T_PwrRamp_hint = "Sanftes Hochfahren, um PSU-Schutz zu vermeiden";


} // end of namespace lang_de_de
