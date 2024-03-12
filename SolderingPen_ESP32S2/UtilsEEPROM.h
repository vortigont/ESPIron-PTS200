#pragma once
#include "config.h"
#include <EEPROM.h>

#define ADDR_SYSTEM_INIT_FLAG 0
#define ADDR_DEFAULT_TEMP ADDR_SYSTEM_INIT_FLAG + 4
#define ADDR_SLEEP_TEMP ADDR_DEFAULT_TEMP + 2
#define ADDR_BOOST_TEMP ADDR_SLEEP_TEMP + 2
#define ADDR_TIME_2_SLEEP ADDR_BOOST_TEMP + 1
#define ADDR_TIME_2_OFF ADDR_TIME_2_SLEEP + 2
#define ADDR_TIME_OF_BOOST ADDR_TIME_2_OFF + 1
#define ADDR_MAIN_SCREEN ADDR_TIME_OF_BOOST + 1
#define ADDR_PID_ENABLE ADDR_MAIN_SCREEN + 1
#define ADDR_BEEP_ENABLE ADDR_PID_ENABLE + 1
#define ADDR_VOLTAGE_VALUE ADDR_BEEP_ENABLE + 1
#define ADDR_QC_ENABLE ADDR_VOLTAGE_VALUE + 1
#define ADDR_WAKEUP_THRESHOLD ADDR_QC_ENABLE + 1
#define ADDR_CURRENT_TIP ADDR_WAKEUP_THRESHOLD + 1
#define ADDR_NUMBER_OF_TIPS ADDR_CURRENT_TIP + 1

#define ADDR_TIP_NAME ADDR_NUMBER_OF_TIPS + 1
#define ADDR_CAL_TEMP ADDR_TIP_NAME + TIPNAMELENGTH *TIPMAX

#define ADDR_LANGUAGE ADDR_CAL_TEMP + 2 * CALNUM *TIPMAX
#define ADDR_HAND_SIDE ADDR_LANGUAGE + 1

#define ADDR_EEPROM_SIZE ADDR_HAND_SIDE + 1


extern uint16_t DefaultTemp;
extern uint16_t SleepTemp;
extern uint8_t BoostTemp;
extern uint16_t time2sleep;
extern uint8_t time2off;
extern uint8_t timeOfBoost;
extern uint8_t MainScrType;
extern bool PIDenable;
extern bool beepEnable;
extern volatile uint8_t VoltageValue;
extern bool QCEnable;
extern uint8_t WAKEUPthreshold;
extern uint8_t CurrentTip;
extern uint8_t NumberOfTips;

extern char TipName[TIPMAX][TIPNAMELENGTH];
extern uint16_t CalTemp[TIPMAX][CALNUM];

extern uint8_t language;
extern uint8_t hand_side;

bool write_default_EEPROM();

bool init_EEPROM();

// writes user settings to EEPROM using updade function to minimize write cycles
// 使用升级功能将用户设置写入EEPROM，以最小化写入周期
bool update_EEPROM();

bool read_EEPROM();

bool update_default_temp_EEPROM();
