#pragma once

// Firmware version
#define FW_VERSION "v0.0.1_experimental"
#define FW_VERSION_NUM 422

// Type of MOSFET
#define P_MOSFET // P_MOSFET or N_MOSFET

// Type of OLED Controller
// #define SSD1306
#define SH1107
//typedef u8g2_uint_t u8g_uint_t;
#define SCREEN_OFFSET     2

// Pins
#define TIP_ADC_SENSOR_PIN  1               // tip temperature sense 烙铁头温感
#define VIN_PIN             6               // input voltage sense 检测输入电压
#define BUZZER_PIN          3               // buzzer 蜂鸣器
#define BUTTON_ACTION       GPIO_NUM_0      // middle push-button
#define BUTTON_INCR         GPIO_NUM_2      // incrementer “+” push-button
#define BUTTON_DECR         GPIO_NUM_4      // decrementer “-” push-button
#define HEATER_PIN          GPIO_NUM_5      // heater MOSFET PWM control 加热器MOSFET PWM控制
#define SH1107_RST_PIN      7               // display reset pin

// CH224K USB PD chip pins connection
// https://components101.com/sites/default/files/component_datasheet/WCH_CH224K_ENG.pdf
#define PD_CFG_1          GPIO_NUM_16
#define PD_CFG_2          GPIO_NUM_17
#define PD_CFG_3          GPIO_NUM_18

#define QC_DP_PIN         14
#define QC_DM_PIN         13

// Heater PWM parameters
#define HEATER_CHANNEL    LEDC_CHANNEL_2     // PWM channel
#define HEATER_FREQ       200   // PWM frequency
#define HEATER_RES        LEDC_TIMER_8_BIT     // PWM resolution

// Default temperature control value (recommended soldering temperature: 300~380°C)
// 默认温度控制值(推荐焊接温度:300~380°C)
#define TEMP_DEFAULT      220   // 默认温度
#define TEMP_MIN          150   // 最小温度
#define TEMP_MAX          450   // 最大温度
#define TEMP_STEP         5     // temperature change step / 旋转编码器温度变化步进
#define TEMP_NOTIP        500   // virtual temperature threshold when tip is not installed (op amp max ~650 C)
#define TEMP_STANDBY      120   // 休眠温度
#define TEMP_STANDBY_MIN  100
#define TEMP_STANDBY_MAX  180
#define TEMP_STANDBY_STEP 10
#define TEMP_BOOST        50    // 升温步进
#define TEMP_BOOST_MIN    30
#define TEMP_BOOST_MAX    100
#define TEMP_BOOST_STEP   10

// Default tip temperature calibration value / 默认的T12烙铁头温度校准值
#define TEMP200           200   // temperature at ADC = 200 
#define TEMP280           280   // temperature at ADC = 280
#define TEMP360           360   // temperature at ADC = 360 
#define TEMPCHP           35    // chip temperature while calibration 校准时芯片温度
#define CALNUM            4     // Calibration point number
#define TIPMAX            8     // max number of tips
#define TIPNAMELENGTH     6     // max length of tip names (including termination)
#define TIPNAME           "PTS  " // default tip name

// Default timeout values, in milliseconds
#define TIMEOUT_STANDBY         60000       // standby time, when heater lowers it's temperature down to standy temp.
#define TIMEOUT_STANDBY_MIN     30          // sec
#define TIMEOUT_STANDBY_MAX     180         // sec
#define TIMEOUT_STANDBY_STEP    15          // sec
#define TIMEOUT_IDLE            300000      // idle mode timeout, when Iron switches off the heater after a certain period of inactivity, ms
#define TIMEOUT_IDLE_MIN        3           // min
#define TIMEOUT_IDLE_MAX        15          // min
#define TIMEOUT_IDLE_STEP       1           // min
#define TIMEOUT_SUSPEND         1200000     // suspend timeout, after this time Iron will go suspend mode, disabling screen and all sensors
#define TIMEOUT_SUSPEND_MIN     15          // min
#define TIMEOUT_SUSPEND_MAX     120         // min
#define TIMEOUT_SUSPEND_STEP    15          // min
#define TIMEOUT_BOOST           60000       // boost mode duration / 停留在加热模式多少秒
#define TIMEOUT_BOOST_MIN       15          // sec
#define TIMEOUT_BOOST_MAX       180         // sec
#define TIMEOUT_BOOST_STEP      15          // sec

// MPU vibration detection accuracy, the smaller the value, the more sensitive it is / MPU 震动检测精度，数值越小，越灵敏
#define WAKEUP_THRESHOLD        10

// Control values
#define SMOOTHIE                0.2         // OpAmp output smoothing coefficient (1=no smoothing; default: 0.05) / OpAmp输出平滑系数 (1=无平滑; 默认：0.05)
#define BEEP_ENABLE             true        // enable/disable buzzer


// MOSFET control definitions
#if defined(P_MOSFET)           // P-Channel MOSFET
#define HEATER_INVERT     false
#elif defined(N_MOSFET)         // N-Channel MOSFET
#define HEATER_INVERT     true
#else
#error Wrong MOSFET type!
#endif

//Language
#define DEFAULT_LANGUAGE  0

//Hand side
#define DEFAULT_HAND_SIDE 1


// timers
#define VIN_REFRESH_INTERVAL    500     // Vin readings period, ms

