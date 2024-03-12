#pragma once

// Firmware version
#define VERSION "v4.5.3" //20240130
#define VERSION_NUM 422

// Type of MOSFET
#define P_MOSFET // P_MOSFET or N_MOSFET

// Type of OLED Controller
// #define SSD1306
#define SH1107
//typedef u8g2_uint_t u8g_uint_t;
#define SCREEN_OFFSET     2

// Type of rotary encoders / 旋转编码器的类型
#define ROTARY_TYPE       0     // 0: 2 increments/step; 1: 4 increments/step (default)
#define BUTTON_DELAY      5

// Pins
#define SENSOR_PIN        1     // tip temperature sense 烙铁头温感
#define VIN_PIN           6     // input voltage sense 检测输入电压
#define BUZZER_PIN        3     // buzzer 蜂鸣器
#define BUTTON_PIN        0     // switch 按键right
#define BUTTON_P_PIN      4     // 1 键位为“+”
#define BUTTON_N_PIN      2     // 2 键位为“-”
#define CONTROL_PIN       5     // heater MOSFET PWM control 加热器MOSFET PWM控制
#define CONTROL_CHANNEL   2     // PWM channel
#define CONTROL_FREQ      200   // PWM frequency
#define CONTROL_HIGHFREQ  1000  // PWM frequency for 20V/50% PWM mode
#define CONTROL_RES       8     // PWM resolution

// CH224K USB PD chip pins connection
// https://components101.com/sites/default/files/component_datasheet/WCH_CH224K_ENG.pdf
#define PD_CFG_0          16
#define PD_CFG_1          17
#define PD_CFG_2          18

#define QC_DP_PIN         14
#define QC_DM_PIN         13

// Default temperature control value (recommended soldering temperature: 300~380°C)
// 默认温度控制值(推荐焊接温度:300~380°C)
#define TEMP_MIN          50    // 最小温度
#define TEMP_MAX          450   // 最大温度
#define TEMP_DEFAULT      260   // 默认温度
#define TEMP_SLEEP        150   // 休眠温度
#define TEMP_BOOST        50    // 升温步进
#define TEMP_STEP         10    // temperature change step / 旋转编码器温度变化步进
#define POWER_LIMIT_15    170   // 功率限制
#define POWER_LIMIT_20    255   // 功率限制
#define POWER_LIMIT_20_2  127   // 功率限制

// Default tip temperature calibration value / 默认的T12烙铁头温度校准值
#define TEMP200           200   // temperature at ADC = 200 
#define TEMP280           280   // temperature at ADC = 280
#define TEMP360           360   // temperature at ADC = 360 
#define TEMPCHP           35    // chip temperature while calibration 校准时芯片温度
#define CALNUM            4     // Calibration point number
#define TIPMAX            8     // max number of tips
#define TIPNAMELENGTH     6     // max length of tip names (including termination)
#define TIPNAME           "PTS  " // default tip name

// Default timer value (0 = disabled) / 默认的定时器值 (0 = 禁用)
#define TIME2SLEEP        60    // sleep mode timer, seconds / 几秒钟后进入睡眠模式
#define TIME2OFF          5     // off timer, minutes / 几分钟后就要关闭加热器了
#define TIMEOFBOOST       60    // boos mode duration / 停留在加热模式多少秒
#define WAKEUP_THRESHOLD  10    // MPU vibration detection accuracy, the smaller the value, the more sensitive it is / MPU 震动检测精度，数值越小，越灵敏

// Control values
#define TIME2SETTLE       5000  // The time in microseconds allowed for the OpAmp output to stabilize / 以微秒为单位的时间允许OpAmp输出稳定
#define TIME2SETTLE_20V   2000  // The time in microseconds allowed for the OpAmp output to stabilize / 以微秒为单位的时间允许OpAmp输出稳定
#define SMOOTHIE          0.05  // OpAmp output smoothing coefficient (1=no smoothing; default: 0.05) / OpAmp输出平滑系数 (1=无平滑; 默认：0.05)
#define PID_ENABLE        false // enable PID control
#define BEEP_ENABLE       true  // enable/disable buzzer
#define VOLTAGE_VALUE     3     // 电压值
#define QC_ENABLE         false // enable/disable QC3.0
#define MAINSCREEN        1     // type of main screen (0: big numbers; 1: more infos)

// EEPROM identifier
#define EEPROM_SIZE       1024

// MOSFET control definitions
#if defined(P_MOSFET)           // P-Channel MOSFET
#define HEATER_ON         255
#define HEATER_OFF        0
#define HEATER_PWM        255 - Output
#elif defined(N_MOSFET)         // N-Channel MOSFET
#define HEATER_ON         0
#define HEATER_OFF        255
#define HEATER_PWM        Output
#else
#error Wrong MOSFET type!
#endif

//Language
#define DEFAULT_LANGUAGE  0

//Hand side
#define DEFAULT_HAND_SIDE 1
