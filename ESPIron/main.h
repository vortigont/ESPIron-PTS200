#pragma once
#include "Arduino.h"


// function forward declarations


void PD_Update();

// creates a short beep on the buzzer
// 在蜂鸣器上创建一个短的哔哔声
void beep();

static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
