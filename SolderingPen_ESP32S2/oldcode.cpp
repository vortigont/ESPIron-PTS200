#ifdef DO_NOT_COMPILE

/*
// controls the heater 控制加热器
void thermostatCheck() {
  // define heater TargetTemp according to current working mode / 根据当前工作模式定义设定值
  if (inOffMode)
    heater.disable();
  else if (inSleepMode)
    heater.setTargetTemp(espIron._temp.sleep);
  else if (inBoostMode) {
    heater.setTargetTemp( constrain(heater.getTargetTemp() + espIron._temp.boost, 0, TEMP_MAX) );
  }

  // refresh displayed temp value
  ShowTemp = heater.getCurrentTemp();

  // set state variable if temperature is in working range; beep if working temperature has been just reached
  // 温度在工作范围内可设置状态变量;当工作温度刚刚达到时，会发出蜂鸣声
  int gap = abs(heater.getTargetTemp() - heater.getCurrentTemp());
  if (gap < 5) {
    if (!isWorky && beepIfWorky) beep();
    isWorky = true;
    beepIfWorky = false;
  } else
    isWorky = false;

  // checks if tip is present or currently inserted
  // 检查烙铁头是否存在或当前已插入
  if (ShowTemp > TEMP_NOTIP) TipIsPresent = false;  // tip removed ? 烙铁头移除？
  if (!TipIsPresent && (ShowTemp < TEMP_NOTIP)) {  // new tip inserted ? 新的烙铁头插入？
    beep();               // beep for info
    TipIsPresent = true;  // tip is present now 烙铁头已经存在
    ChangeTipScreen();    // show tip selection screen 显示烙铁头选择屏幕
    update_EEPROM();      // update setting in EEPROM EEPROM的更新设置
    //handleMoved = true;   // reset all timers 重置所有计时器
    c0 = LOW;             // switch must be released 必须松开开关
    setRotary(TEMP_MIN, TEMP_MAX, TEMP_STEP, heater.getTargetTemp());  // reset rotary encoder 重置旋转编码器
  }
}
*/

// sets start values for rotary encoder 设置旋转编码器的起始值
void setRotary(int rmin, int rmax, int rstep, int rvalue) {
  countMin = rmin << ROTARY_TYPE;
  countMax = rmax << ROTARY_TYPE;
  countStep = rstep;
  count = rvalue << ROTARY_TYPE;
}

// reads current rotary encoder value 读取当前旋转编码器值
int getRotary() {
  Button_loop();
  return (count >> ROTARY_TYPE);
}


// draws the main screen 绘制主屏幕
void MainScreen() {
  u8g2.firstPage();
  do {
    // u8g2.setCursor(0, 0);
    // u8g2.print(F("nihao"));
    //  draw heater.getTargetTemp() temperature
    u8g2.setFont(PTS200_16);
    if(language == 2){
      u8g2.setFont(u8g2_font_unifont_t_chinese3);
    }
    u8g2.setFontPosTop();
    //    u8g2.drawUTF8(0, 0 + SCREEN_OFFSET, "设温:");
    u8g2.drawUTF8(0, 0 + SCREEN_OFFSET, txt_set_temp[language]);
    u8g2.setCursor(40, 0 + SCREEN_OFFSET);
    u8g2.setFont(u8g2_font_unifont_t_chinese3);
    u8g2.print(heater.getTargetTemp(), 0);

    u8g2.setFont(PTS200_16);
    if(language == 2){
      u8g2.setFont(u8g2_font_unifont_t_chinese3);
    }
    // draw status of heater 绘制加热器状态
    u8g2.setCursor(96, 0 + SCREEN_OFFSET);
    if (ShowTemp > TEMP_NOTIP)
      u8g2.print(txt_error[language]);
    else if (inOffMode)
      u8g2.print(txt_off[language]);
    else if (inSleepMode)
      u8g2.print(txt_sleep[language]);
    else if (inBoostMode)
      u8g2.print(txt_boost[language]);
    else if (isWorky)
      u8g2.print(txt_worky[language]);
    else if (heater.getCurrentTemp() - heater.getTargetTemp() < PID_ENGAGE_DIFF)
      u8g2.print(txt_on[language]);
    else
      u8g2.print(txt_hold[language]);

    u8g2.setFont(u8g2_font_unifont_t_chinese3);
    // rest depending on main screen type 休息取决于主屏幕类型
    if (MainScrType) {
      // draw current tip and input voltage 绘制当前烙铁头及输入电压
      newSENSORTmp = newSENSORTmp + 0.01 * accel.getAccellTemp();
      SENSORTmpTime++;
      if (SENSORTmpTime >= 100) {
        lastSENSORTmp = newSENSORTmp;
        newSENSORTmp = 0;
        SENSORTmpTime = 0;
      }
      u8g2.setCursor(0, 50);
      u8g2.print(lastSENSORTmp, 1);
      u8g2.print(F("C"));
      u8g2.setCursor(83, 50);
      u8g2.print(inputVoltage/1000, 1);  // convert mv in V
      u8g2.print(F("V"));
      // draw current temperature 绘制当前温度
      u8g2.setFont(u8g2_font_freedoomr25_tn);
      u8g2.setFontPosTop();
      u8g2.setCursor(37, 18);
      if (ShowTemp > 500)
        u8g2.print("Err");
      else
        u8g2.printf("%03d", ShowTemp);
    } else {
      // draw current temperature in big figures 用大数字绘制当前温度
      u8g2.setFont(u8g2_font_fub42_tn);
      u8g2.setFontPosTop();
      u8g2.setCursor(15, 20);
      if (ShowTemp > 500)
        u8g2.print("Err");
      else
        u8g2.printf("%03d", ShowTemp);
    }
  } while (u8g2.nextPage());
}

// setup screen 设置屏幕
void SetupScreen() {
  heater.disable();
  beep();
  uint8_t selection = 0;
  bool repeat = true;

  while (repeat) {
    selection = MenuScreen(SetupItems, sizeof(SetupItems), selection);
    switch (selection) {
      case 0: {
        TipScreen();
        repeat = false;
      } break;
      case 1: {
        TempScreen();
      } break;
      case 2: {
        TimerScreen();
      } break;
        //      case 3:
        //        PIDenable = MenuScreen(ControlTypeItems,
        //        sizeof(ControlTypeItems), PIDenable); break;
      case 3: {
        MainScrType =
            MenuScreen(MainScreenItems, sizeof(MainScreenItems), MainScrType);
      } break;
      case 4: {
        InfoScreen();
      } break;
      case 5:
        VoltageValue =
            MenuScreen(VoltageItems, sizeof(VoltageItems), VoltageValue);
        PD_Update();
        break;
      case 6:
        QCEnable = MenuScreen(QCItems, sizeof(QCItems), QCEnable);
        break;
      case 7:
        beepEnable = MenuScreen(BuzzerItems, sizeof(BuzzerItems), beepEnable);
        break;
      case 8: {
        restore_default_config = MenuScreen(DefaultItems, sizeof(DefaultItems),
                                            restore_default_config);
        if (restore_default_config) {
          restore_default_config = false;
          write_default_EEPROM();
          read_EEPROM();
        }
      } break;
      case 9: {
        bool lastbutton = (!digitalRead(BUTTON_PIN));
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(0, 10,
                     "MSC Update");  // write something to the internal memory
        u8g2.sendBuffer();           // transfer internal memory to the display
        delay(1000);
        do {
          MSC_Update.onEvent(usbEventCallback);
          MSC_Update.begin();
          if (lastbutton && digitalRead(BUTTON_PIN)) {
            delay(10);
            lastbutton = false;
          }
        } while (digitalRead(BUTTON_PIN) || lastbutton);

        MSC_Update.end();
      } break;
      case 10: {
        Serial.println(language);
        language = MenuScreen(LanguagesItems, sizeof(LanguagesItems), language);
        Serial.println(language);
        repeat = false;
      } break;
      case 11: {
        if(hand_side == 0){
          u8g2.setDisplayRotation(U8G2_R3);
          hand_side = 1;
        }else{
          u8g2.setDisplayRotation(U8G2_R1);
          hand_side = 0;
        }
        repeat = false;
      } break;
      default:
        repeat = false;
        break;
    }
  }
  update_EEPROM();
  heater.enable();
  setRotary(TEMP_MIN, TEMP_MAX, TEMP_STEP, heater.getTargetTemp());
}

// tip settings screen 烙铁头设置屏幕
void TipScreen() {
  uint8_t selection = 0;
  bool repeat = true;
  while (repeat) {
    selection = MenuScreen(TipItems, sizeof(TipItems), selection);
    switch (selection) {
      case 0:
        ChangeTipScreen();
        break;
      case 1:
        CalibrationScreen();
        break;
      case 2:
        InputNameScreen();
        break;
      case 3:
        DeleteTipScreen();
        break;
      case 4:
        AddTipScreen();
        break;
      default:
        repeat = false;
        break;
    }
  }
}

// temperature settings screen 温度设置屏幕
void TempScreen() {
  uint8_t selection = 0;
  bool repeat = true;
  while (repeat) {
    selection = MenuScreen(TempItems, sizeof(TempItems), selection);
    switch (selection) {
      case 0:
        setRotary(TEMP_MIN, TEMP_MAX, TEMP_STEP, espIron._temp.working);
        espIron.setTempWorking(InputScreen(DefaultTempItems));
        break;
      case 1:
        setRotary(50, TEMP_MAX, TEMP_STEP, espIron._temp.sleep);
        espIron.setTempSleep( InputScreen(SleepTempItems) );
        break;
      case 2:
        setRotary(10, 100, TEMP_STEP, espIron._temp.boost);
        espIron.setTempBoost( InputScreen(BoostTempItems) );
        break;
      default:
        repeat = false;
        break;
    }
  }
}

// timer settings screen 定时器设置屏幕
void TimerScreen() {
  uint8_t selection = 0;
  bool repeat = true;
  while (repeat) {
    selection = MenuScreen(TimerItems, sizeof(TimerItems), selection);
    switch (selection) {
      case 0:
        setRotary(0, 600, 10, espIron._timeout.sleep);
        espIron.setTimeOutSleep( InputScreen(SleepTimerItems) );
        break;
      case 1:
        setRotary(0, 60, 1, espIron._timeout.off);
        espIron.setTimeOutOff( InputScreen(OffTimerItems) );
        break;
      case 2:
        setRotary(0, 180, 10, espIron._timeout.boost);
        espIron.setTimeOutBoost( InputScreen(BoostTimerItems) );
        break;
      case 3: {
        // todo: get this from NVS
        setRotary(0, 50, 5, WAKEUPthreshold);
        WAKEUPthreshold = InputScreen(WAKEUPthresholdItems);
        accel.setMotionThreshold(WAKEUPthreshold);
        break;
      }
      default:
        repeat = false;
        break;
    }
  }
}

// menu screen 菜单屏幕
uint8_t MenuScreen(const char *Items[][language_types], uint8_t numberOfItems,
                   uint8_t selected) {
  // Serial.println(numberOfItems);
  bool isTipScreen = ((strcmp(Items[0][language], "烙铁头:") == 0) ||
                      (strcmp(Items[0][language], "Tip:") == 0) ||
                      (strcmp(Items[0][language], "烙鐵頭:") == 0));
  uint8_t lastselected = selected;
  int8_t arrow = 0;
  if (selected) arrow = 1;
  numberOfItems = numberOfItems / language_types;
  numberOfItems >>= 2;

  // 根据OLED控制器设置选择方向
#if defined(SSD1306)
  setRotary(0, numberOfItems + 3, 1, selected);
#elif defined(SH1107)
  setRotary(0, numberOfItems - 2, 1, selected);
#else
#error Wrong OLED controller type!
#endif

  bool lastbutton = (!digitalRead(BUTTON_PIN));

  do {
    selected = getRotary();
    arrow = constrain(arrow + selected - lastselected, 0, 2);
    lastselected = selected;
    u8g2.firstPage();
    do {
      u8g2.setFont(PTS200_16);
      if(language == 2){
        u8g2.setFont(u8g2_font_unifont_t_chinese3);
      }
      u8g2.setFontPosTop();
      u8g2.drawUTF8(0, 0 + SCREEN_OFFSET, Items[0][language]);
      if (isTipScreen)
        u8g2.drawUTF8(54, 0 + SCREEN_OFFSET, TipName[CurrentTip]);
      u8g2.drawUTF8(0, 16 * (arrow + 1) + SCREEN_OFFSET, ">");
      for (uint8_t i = 0; i < 3; i++) {
        uint8_t drawnumber = selected + i + 1 - arrow;
        if (drawnumber < numberOfItems)
          u8g2.drawUTF8(12, 16 * (i + 1) + SCREEN_OFFSET,
                        Items[selected + i + 1 - arrow][language]);
      }
    } while (u8g2.nextPage());
    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);

  beep();
  return selected;
}

void MessageScreen(const char *Items[][language_types], uint8_t numberOfItems) {
  numberOfItems = numberOfItems / language_types;
  bool lastbutton = (!digitalRead(BUTTON_PIN));
  u8g2.firstPage();
  do {
    u8g2.setFont(PTS200_16);
        if(language == 2){
      u8g2.setFont(u8g2_font_unifont_t_chinese3);
    }
    u8g2.setFontPosTop();
    for (uint8_t i = 0; i < numberOfItems; i++)
      u8g2.drawUTF8(0, i * 16, Items[i][language]);
  } while (u8g2.nextPage());
  do {
    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);
  beep();
}

// input value screen 输入值屏幕
uint16_t InputScreen(const char *Items[][language_types]) {
  uint16_t value;
  bool lastbutton = (!digitalRead(BUTTON_PIN));

  do {
    value = getRotary();
    u8g2.firstPage();
    do {
      u8g2.setFont(PTS200_16);
          if(language == 2){
      u8g2.setFont(u8g2_font_unifont_t_chinese3);
    }
      u8g2.setFontPosTop();
      u8g2.drawUTF8(0, 0 + SCREEN_OFFSET, Items[0][language]);
      u8g2.setCursor(0, 32);
      u8g2.print(">");
      u8g2.setCursor(10, 32);
      if (value == 0)
        u8g2.print(txt_Deactivated[language]);
      else {
        u8g2.print(value);
        u8g2.print(" ");
        u8g2.print(Items[1][language]);
      }
    } while (u8g2.nextPage());
    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);

  beep();
  return value;
}

// information display screen 信息显示屏幕
void InfoScreen() {
  bool lastbutton = (!digitalRead(BUTTON_PIN));

  do {
    float fTmp = accel.getAccellTemp();      // read cold junction temperature
    u8g2.firstPage();
    do {
      u8g2.setFont(PTS200_16);
      if(language == 2){
        u8g2.setFont(u8g2_font_unifont_t_chinese3);
      }
      u8g2.setFontPosTop();
      u8g2.setCursor(0, 0 + SCREEN_OFFSET);
      u8g2.print(txt_temp[language]);
      u8g2.print(fTmp, 1);
      u8g2.print(F(" C"));
      u8g2.setCursor(0, 16 + SCREEN_OFFSET);
      u8g2.print(txt_voltage[language]);
      u8g2.print(getVIN()/1000, 1);               // convert mv in V
      u8g2.print(F(" V"));
      u8g2.setCursor(0, 16 * 2 + SCREEN_OFFSET);
      u8g2.print(txt_Version[language]);
      u8g2.print(VERSION);
      // u8g2.setCursor(0, 48); u8g2.print(F("IMU:  "));
      // u8g2.print(accelerometer[1], DEC); u8g2.print(F(""));
    } while (u8g2.nextPage());

    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);

  beep();
}

// change tip screen 改变烙铁头屏幕
void ChangeTipScreen() {
  uint8_t selected = CurrentTip;
  uint8_t lastselected = selected;
  int8_t arrow = 0;
  if (selected) arrow = 1;
  setRotary(0, NumberOfTips - 1, 1, selected);
  bool lastbutton = (!digitalRead(BUTTON_PIN));

  Serial.print("selected: ");
  Serial.println(selected);
  Serial.print("lastselected: \n");
  Serial.println(lastselected);
  Serial.print("NumberOfTips: \n");
  Serial.println(NumberOfTips);

  do {
    selected = getRotary();
    arrow = constrain(arrow + selected - lastselected, 0, 2);
    lastselected = selected;
    u8g2.firstPage();
    do {
      u8g2.setFont(PTS200_16);
          if(language == 2){
      u8g2.setFont(u8g2_font_unifont_t_chinese3);
    }
      u8g2.setFontPosTop();
      //      strcpy_P(F_Buffer, PSTR("选择烙铁头"));
      u8g2.drawUTF8(0, 0 + SCREEN_OFFSET, txt_select_tip[language]);
      u8g2.drawUTF8(0, 16 * (arrow + 1) + SCREEN_OFFSET, ">");
      for (uint8_t i = 0; i < 3; i++) {
        uint8_t drawnumber = selected + i - arrow;
        if (drawnumber < NumberOfTips)
          u8g2.drawUTF8(12, 16 * (i + 1) + SCREEN_OFFSET,
                        TipName[selected + i - arrow]);
      }
    } while (u8g2.nextPage());
    if (lastbutton && digitalRead(BUTTON_PIN)) {
      delay(10);
      lastbutton = false;
    }
  } while (digitalRead(BUTTON_PIN) || lastbutton);

  beep();
  CurrentTip = selected;
}

// temperature calibration screen 温度校准屏幕
void CalibrationScreen() {
  uint16_t CalTempNew[4];
  uint16_t lastTargetTemp = heater.getTargetTemp();
  for (uint8_t CalStep = 0; CalStep < 3; CalStep++) {
    heater.setTargetTemp( CalTemp[CurrentTip][CalStep] );
    LOGI(T_CTRL, print, "Target Temp: ");
    LOGI(T_CTRL, println, heater.getTargetTemp());
    setRotary(100, 500, 1, heater.getTargetTemp());
    beepIfWorky = true;
    bool lastbutton = (!digitalRead(BUTTON_PIN));

    do {
      thermostatCheck();   // heater control

      u8g2.firstPage();
      do {
        u8g2.setFont(PTS200_16);
            if(language == 2){
      u8g2.setFont(u8g2_font_unifont_t_chinese3);
    }
        u8g2.setFontPosTop();
        //        strcpy_P(F_Buffer, PSTR("校准"));
        u8g2.drawUTF8(0, 0 + SCREEN_OFFSET, txt_calibrate[language]);
        u8g2.setCursor(0, 16 + SCREEN_OFFSET);
        u8g2.print(txt_step[language]);
        u8g2.print(CalStep + 1);
        u8g2.print(" of 3");
        if (isWorky) {
          u8g2.setCursor(0, 32 + SCREEN_OFFSET);
          u8g2.print(txt_set_measured[language]);
          u8g2.setCursor(0, 48 + SCREEN_OFFSET);
          u8g2.print(txt_s_temp[language]);
          u8g2.print(getRotary());
        } else {
          u8g2.setCursor(0, 32 + SCREEN_OFFSET);
          u8g2.print(txt_temp_2[language]);
          u8g2.print(heater.getCurrentTemp());
          u8g2.setCursor(0, 48 + SCREEN_OFFSET);
          u8g2.print(txt_wait_pls[language]);
        }
      } while (u8g2.nextPage());
      if (lastbutton && digitalRead(BUTTON_PIN)) {
        delay(10);
        lastbutton = false;
      }
    } while (digitalRead(BUTTON_PIN) || lastbutton);

    CalTempNew[CalStep] = getRotary();
    beep();
    delay(10);
  }

  heater.disable();  // shut off heater 关闭加热器
  if (VoltageValue == 3) {
    delayMicroseconds(TIME2SETTLE_20V);
  } else {
    delayMicroseconds(TIME2SETTLE);  // wait for voltage to settle 等待电压稳定
  }
  CalTempNew[3] = accel.getAccellTemp();  // read chip temperature 读芯片温度
  if ((CalTempNew[0] + 10 < CalTempNew[1]) &&
      (CalTempNew[1] + 10 < CalTempNew[2])) {
    if (MenuScreen(StoreItems, sizeof(StoreItems), 0)) {
      for (uint8_t i = 0; i < 4; i++) CalTemp[CurrentTip][i] = CalTempNew[i];
    }
  }

  heater.setTargetTemp( lastTargetTemp );
  update_EEPROM();
}

// input tip name screen 输入烙铁头名字屏幕
void InputNameScreen() {
  uint8_t value;

  for (uint8_t digit = 0; digit < (TIPNAMELENGTH - 1); digit++) {
    bool lastbutton = (!digitalRead(BUTTON_PIN));
    setRotary(31, 96, 1, 65);
    do {
      value = getRotary();
      if (value == 31) {
        value = 95;
        setRotary(31, 96, 1, 95);
      }
      if (value == 96) {
        value = 32;
        setRotary(31, 96, 1, 32);
      }
      u8g2.firstPage();
      do {
        u8g2.setFont(PTS200_16);
            if(language == 2){
      u8g2.setFont(u8g2_font_unifont_t_chinese3);
    }
        u8g2.setFontPosTop();
        u8g2.drawUTF8(0, 0 + SCREEN_OFFSET, txt_enter_tip_name[language]);
        u8g2.setCursor(12 * digit, 48 + SCREEN_OFFSET);
        u8g2.print(char(94));
        u8g2.setCursor(0, 32 + SCREEN_OFFSET);
        for (uint8_t i = 0; i < digit; i++) u8g2.print(TipName[CurrentTip][i]);
        u8g2.setCursor(12 * digit, 32 + SCREEN_OFFSET);
        u8g2.print(char(value));
      } while (u8g2.nextPage());
      if (lastbutton && digitalRead(BUTTON_PIN)) {
        delay(10);
        lastbutton = false;
      }
    } while (digitalRead(BUTTON_PIN) || lastbutton);
    TipName[CurrentTip][digit] = value;
    beep();
    delay(10);
  }
  TipName[CurrentTip][TIPNAMELENGTH - 1] = 0;
  return;
}

// delete tip screen 删除烙铁头屏幕
void DeleteTipScreen() {
  if (NumberOfTips == 1) {
    MessageScreen(DeleteMessage, sizeof(DeleteMessage));
  } else if (MenuScreen(SureItems, sizeof(SureItems), 0)) {
    if (CurrentTip == (NumberOfTips - 1)) {
      CurrentTip--;
    } else {
      for (uint8_t i = CurrentTip; i < (NumberOfTips - 1); i++) {
        for (uint8_t j = 0; j < TIPNAMELENGTH; j++)
          TipName[i][j] = TipName[i + 1][j];
        for (uint8_t j = 0; j < 4; j++) CalTemp[i][j] = CalTemp[i + 1][j];
      }
    }
    NumberOfTips--;
  }
}

// add new tip screen 添加新的烙铁头屏幕
void AddTipScreen() {
  if (NumberOfTips < TIPMAX) {
    CurrentTip = NumberOfTips++;
    InputNameScreen();
    CalTemp[CurrentTip][0] = TEMP200;
    CalTemp[CurrentTip][1] = TEMP280;
    CalTemp[CurrentTip][2] = TEMP360;
    CalTemp[CurrentTip][3] = TEMPCHP;
  } else
    MessageScreen(MaxTipMessage, sizeof(MaxTipMessage));
}

unsigned int Button_Time1 = 0, Button_Time2 = 0;

void Button_loop() {
  // '-' decrementing button
  if (!digitalRead(BUTTON_N_PIN) && a0 == 1) {
    delay(BUTTON_DELAY);
    if (!digitalRead(BUTTON_N_PIN)) {
      int count0 = count;
      count = constrain(count + countStep, countMin, countMax);
      if (!(countMin == TEMP_MIN && countMax == TEMP_MAX)) {
        if (count0 + countStep > countMax) {
          count = countMin;
        }
      }
      a0 = 0;
    }
  } else if (!digitalRead(BUTTON_N_PIN) && a0 == 0) {
    delay(BUTTON_DELAY);
    if (Button_Time1 > 10)  // this value controls long-press timeout / 这里的数越大，需要长按时间更长
      count = constrain(count + countStep, countMin, countMax);
    else
      Button_Time1++;
  } else if (digitalRead(BUTTON_N_PIN)) {
    Button_Time1 = 0;
    a0 = 1;
  }

  // '+' incrementing button
  if (!digitalRead(BUTTON_P_PIN) && b0 == 1) {
    delay(BUTTON_DELAY);
    if (!digitalRead(BUTTON_P_PIN)) {
      int count0 = count;
      count = constrain(count - countStep, countMin, countMax);
      if (!(countMin == TEMP_MIN && countMax == TEMP_MAX)) {
        if (count0 - countStep < countMin) {
          count = countMax;
        }
      }
      b0 = 0;
    }
  } else if (!digitalRead(BUTTON_P_PIN) && b0 == 0) {
    delay(BUTTON_DELAY);
    if (Button_Time2 > 10)  // this value controls long-press timeout / 这里的数越大，需要长按时间更长
      count = constrain(count - countStep, countMin, countMax);
    else
      Button_Time2++;
  } else if (digitalRead(BUTTON_P_PIN)) {
    Button_Time2 = 0;
    b0 = 1;
  }
}

// uint16_t calibrate_adc(adc_unit_t adc, adc_atten_t channel) {
//   uint16_t vref;
//   esp_adc_cal_characteristics_t adc_chars;
//   esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)adc,
//   (adc_atten_t)channel, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100,
//   &adc_chars);
//   //Check type of calibration value used to characterize ADC
//   if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
//     Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
//     Serial.println();
//     vref = adc_chars.vref;
//   } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
//     Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n",
//     adc_chars.coeff_a, adc_chars.coeff_b); Serial.println();
//   } else {
//     Serial.println("Default Vref: 1100mV");
//   }
//   return vref;
// }


#endif