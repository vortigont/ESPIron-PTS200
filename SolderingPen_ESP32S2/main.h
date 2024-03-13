#pragma once
#include "Arduino.h"
#include "Languages.h"


// function forward declarations

void getEEPROM();

void PD_Update();

uint16_t getVIN();

/**
 * @brief Averaging 32 ADC readings to reduce noise
 *  对32个ADC读数进行平均以降噪
 *  VP+_Ru = 100k, Rd_GND = 1K
 * 
 */
float denoiseAnalog(byte port);

// calculates real temperature value according to ADC reading and calibration values
// 根据ADC读数和校准值，计算出真实的温度值
void calculateTemp();

// reads current rotary encoder value 读取当前旋转编码器值
int getRotary();

// sets start values for rotary encoder
// 设置旋转编码器的起始值
void setRotary(int rmin, int rmax, int rstep, int rvalue);

// creates a short beep on the buzzer
// 在蜂鸣器上创建一个短的哔哔声
void beep();


/**
 * @brief Read SENSOR internal temperature
 * 读取SENSOR内部温度
 * @return double 
 */
float getAccellTemp();

/**
 * @brief check rotary encoder; set temperature, toggle boost mode, enter setup menu// accordingly
 *  检查旋转编码器;设置温度，切换升压模式，进入设置菜单相应
 * 
 */
void ROTARYCheck();

/**
 * @brief check and activate/deactivate sleep modes
 * 检查和激活/关闭睡眠模式
 * 
 */
void SLEEPCheck();
 
/**
 * @brief reads temperature, vibration switch and supply voltages
 * 读取温度，振动开关和电源电压
 */
void SENSORCheck();

/**
 * @brief measure tip temperature
 * this will disable PWM!!!
 */
void measureTipTemp();

/**
 * @brief input value screen
 * 输入值屏幕
 * 
 * @param Items 
 * @return uint16_t 
 */
uint16_t InputScreen(const char *Items[][language_types]);

/**
 * @brief // menu screen
 * 菜单屏幕
 * @param Items 
 * @param numberOfItems 
 * @param selected 
 * @return uint8_t 
 */
uint8_t MenuScreen(const char *Items[][language_types], uint8_t numberOfItems, uint8_t selected);

/**
 * @brief input value screen
 * 输入值屏幕
 * @param Items 
 * @return uint16_t 
 */
uint16_t InputScreen(const char *Items[][language_types]);

/**
 * @brief controls the heater
 * 控制加热器
 */
void Thermostat();

/**
 * @brief draws the main screen
 * 绘制主屏幕
 * 
 */
void MainScreen();

// setup screen 设置屏幕
void SetupScreen();

// tip settings screen 烙铁头设置屏幕
void TipScreen();

// temperature settings screen 温度设置屏幕
void TempScreen();

// timer settings screen 定时器设置屏幕
void TimerScreen();

// menu screen 菜单屏幕
uint8_t MenuScreen(const char *Items[][language_types], uint8_t numberOfItems, uint8_t selected);

// change tip screen 改变烙铁头屏幕
void ChangeTipScreen();

// temperature calibration screen 温度校准屏幕
void CalibrationScreen();

// input tip name screen 输入烙铁头名字屏幕
void InputNameScreen();

// delete tip screen 删除烙铁头屏幕
void DeleteTipScreen();

// add new tip screen 添加新的烙铁头屏幕
void AddTipScreen();

// information display screen 信息显示屏幕
void InfoScreen();

void Button_loop();

static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
