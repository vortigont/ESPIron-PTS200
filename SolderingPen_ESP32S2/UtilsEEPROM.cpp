#include "UtilsEEPROM.h"

bool write_default_EEPROM(){
  Serial.println("Writing default config to EEPROM");
/*
  EEPROM.writeUShort(ADDR_DEFAULT_TEMP, TEMP_DEFAULT);
  EEPROM.writeUShort(ADDR_SLEEP_TEMP, TEMP_SLEEP);
  EEPROM.writeUChar(ADDR_BOOST_TEMP, TEMP_BOOST);
  EEPROM.writeUShort(ADDR_TIME_2_SLEEP, TIME2SLEEP);
  EEPROM.writeUChar(ADDR_TIME_2_OFF, TIME2OFF);
  EEPROM.writeUChar(ADDR_TIME_OF_BOOST, TIMEOFBOOST);
*/
  EEPROM.writeUChar(ADDR_MAIN_SCREEN, MAINSCREEN);
//  EEPROM.writeBool(ADDR_PID_ENABLE, PID_ENABLE);
  EEPROM.writeBool(ADDR_BEEP_ENABLE, BEEP_ENABLE);
  EEPROM.writeUChar(ADDR_VOLTAGE_VALUE, VOLTAGE_VALUE);
  EEPROM.writeBool(ADDR_QC_ENABLE, QC_ENABLE);
  EEPROM.writeUChar(ADDR_WAKEUP_THRESHOLD, WAKEUP_THRESHOLD);
  EEPROM.writeUChar(ADDR_CURRENT_TIP, 0);
  EEPROM.writeUChar(ADDR_NUMBER_OF_TIPS, 1);

  CalTemp[0][0] = TEMP200;
  CalTemp[0][1] = TEMP280;
  CalTemp[0][2] = TEMP360;
  CalTemp[0][3] = TEMPCHP;
  //  TipName[0][TIPNAMELENGTH] = {TIPNAME};

  for (uint8_t i = 0; i < 1; i++)
  {
    EEPROM.writeString(ADDR_TIP_NAME + i * TIPNAMELENGTH, TipName[i]);
    for (uint8_t j = 0; j < CALNUM; j++)
    {
      EEPROM.writeUShort(ADDR_CAL_TEMP + i * 2 * CALNUM + j * 2, CalTemp[i][j]);
    }
  }

  EEPROM.writeUChar(ADDR_LANGUAGE, DEFAULT_LANGUAGE);
  EEPROM.writeUChar(ADDR_HAND_SIDE, DEFAULT_HAND_SIDE);

  EEPROM.writeUInt(ADDR_SYSTEM_INIT_FLAG, FW_VERSION_NUM);

  if (EEPROM.commit())
  {
    Serial.println("Default config Done");
    return true;
  }
  else
  {
    Serial.println("Default config Failed");
    return false;
  }
}

bool init_EEPROM(){
  Serial.println("Initialising EEPROM");
  if (!EEPROM.begin(ADDR_EEPROM_SIZE))
  {
    Serial.println("Failed to initialise EEPROM");
    //    Serial.println("Restarting...");
    //    delay(1000);
    //    ESP.restart();
    return false;
  }
  Serial.println("EEPROM Done");
  return true;
}

bool update_EEPROM(){
  Serial.println("Updating EEPROM");
/*
  EEPROM.writeUShort(ADDR_DEFAULT_TEMP, DefaultTemp);
  EEPROM.writeUShort(ADDR_SLEEP_TEMP, SleepTemp);
  EEPROM.writeUChar(ADDR_BOOST_TEMP, BoostTemp);
  EEPROM.writeUShort(ADDR_TIME_2_SLEEP, time2sleep);
  EEPROM.writeUChar(ADDR_TIME_2_OFF, time2off);
  EEPROM.writeUChar(ADDR_TIME_OF_BOOST, timeOfBoost);
*/
  EEPROM.writeUChar(ADDR_MAIN_SCREEN, MainScrType);
//  EEPROM.writeBool(ADDR_PID_ENABLE, PIDenable);
  EEPROM.writeBool(ADDR_BEEP_ENABLE, beepEnable);
//  EEPROM.writeUChar(ADDR_VOLTAGE_VALUE, VoltageValue);
  EEPROM.writeBool(ADDR_QC_ENABLE, QCEnable);
  //EEPROM.writeUChar(ADDR_WAKEUP_THRESHOLD, WAKEUPthreshold);
  EEPROM.writeUChar(ADDR_CURRENT_TIP, CurrentTip);
  EEPROM.writeUChar(ADDR_NUMBER_OF_TIPS, NumberOfTips);

  for (uint8_t i = 0; i < NumberOfTips; i++)
  {
    EEPROM.writeString(ADDR_TIP_NAME + i * TIPNAMELENGTH, TipName[i]);
    for (uint8_t j = 0; j < CALNUM; j++)
    {
      EEPROM.writeUShort(ADDR_CAL_TEMP + i * 2 * CALNUM + j * 2, CalTemp[i][j]);
    }
  }

  EEPROM.writeUChar(ADDR_LANGUAGE, language);
  EEPROM.writeUChar(ADDR_HAND_SIDE, hand_side);

  EEPROM.writeUInt(ADDR_SYSTEM_INIT_FLAG, FW_VERSION_NUM);

  if (EEPROM.commit())
  {
    Serial.println("EEPROM Update Done");
    return true;
  }
  else
  {
    Serial.println("EEPROM Update Failed");
    return false;
  }
}

bool read_EEPROM(){
  Serial.println("Reading EEPROM");

  //  write_default_EEPROM();

  //  system_init_flag = EEPROM.readUInt(ADDR_SYSTEM_INIT_FLAG);

  if (EEPROM.readUInt(ADDR_SYSTEM_INIT_FLAG) != FW_VERSION_NUM)
  {
    //    return false;
    Serial.println("System didn't initialised");
    write_default_EEPROM();
  }

/*
  DefaultTemp = EEPROM.readUShort(ADDR_DEFAULT_TEMP);
  SleepTemp = EEPROM.readUShort(ADDR_SLEEP_TEMP);
  BoostTemp = EEPROM.readUChar(ADDR_BOOST_TEMP);
  time2sleep = EEPROM.readUShort(ADDR_TIME_2_SLEEP);
  time2off = EEPROM.readUChar(ADDR_TIME_2_OFF);
  timeOfBoost = EEPROM.readUChar(ADDR_TIME_OF_BOOST);
*/
  MainScrType = EEPROM.readUChar(ADDR_MAIN_SCREEN);
//  PIDenable = EEPROM.readBool(ADDR_PID_ENABLE);
  beepEnable = EEPROM.readBool(ADDR_BEEP_ENABLE);
//  VoltageValue = EEPROM.readUChar(ADDR_VOLTAGE_VALUE);
  QCEnable = EEPROM.readBool(ADDR_QC_ENABLE);
  //WAKEUPthreshold = EEPROM.readUChar(ADDR_WAKEUP_THRESHOLD);
  CurrentTip = EEPROM.readUChar(ADDR_CURRENT_TIP);
  NumberOfTips = EEPROM.readUChar(ADDR_NUMBER_OF_TIPS);

  for (uint8_t i = 0; i < NumberOfTips; i++)
  {
    EEPROM.readString(ADDR_TIP_NAME + i * TIPNAMELENGTH).toCharArray(TipName[i], sizeof(TipName[i]));
    for (uint8_t j = 0; j < CALNUM; j++)
    {
      CalTemp[i][j] = EEPROM.readUShort(ADDR_CAL_TEMP + i * 2 * CALNUM + j * 2);
    }
  }

  language = EEPROM.readUChar(ADDR_LANGUAGE);
  hand_side = EEPROM.readUChar(ADDR_HAND_SIDE);

  return true;
}

bool update_default_temp_EEPROM(){
  return true;
  Serial.println("Updating default temp in EEPROM");

  //EEPROM.writeUShort(ADDR_DEFAULT_TEMP, DefaultTemp);

  if (EEPROM.commit())
  {
    Serial.println("Default temp Update Done");
    return true;
  }
  else
  {
    Serial.println("Default temp Update Failed");
    return false;
  }
}


