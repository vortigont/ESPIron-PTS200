//
#include "const.h"
#include "ironcontroller.hpp"
#include "heater.hpp"
#include "sensors.hpp"
#include "hid.hpp"
#include "main.h"
#include "log.h"

#ifdef CONFIG_TINYUSB_MSC_ENABLED
#include <QC3Control.h>
#include "FirmwareMSC.h"
#include "USB.h"
QC3Control QC(QC_DP_PIN, QC_DM_PIN);
FirmwareMSC MSC_Update;
#endif

#ifndef DEVELOP_MODE
// Iron tip heater object
TipHeater heater(HEATER_PIN, HEATER_CHANNEL, HEATER_INVERT);

// acceleration sensor
GyroSensor accel;
// input voltage sensor
VinSensor vin;
#endif  // DEVELOP_MODE

// Default value can be changed by user and stored in EEPROM
// 用户可以更改并存储在EEPROM中的默认值
bool beepEnable = BEEP_ENABLE;
bool QCEnable = QC_ENABLE;

// MSC Firmware
bool MSC_Updating_Flag = false;




// *** Arduino setup ***
void setup() {
  // set PD-trigger pins
  pinMode(PD_CFG_0, OUTPUT);
  pinMode(PD_CFG_1, OUTPUT);
  pinMode(PD_CFG_2, OUTPUT);

  // tip sensor pin
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  // buzzer in
  pinMode(BUZZER_PIN, OUTPUT);

  // start PD trigger with default 5 volts
  digitalWrite(PD_CFG_0, HIGH);

  Serial.begin(115200);
#ifdef ARDUINO_USB_MODE
  Serial.setTxTimeoutMs(0);
#endif

#if PTS200_DEBUG_LEVEL > 3
  // let ACM device intitialize to catch early log output
  delay(3000);
#endif
  LOGI(T_IRON, printf, "ESPIron PTS200 firmware version: %s\n", FW_VERSION);

  // Start event loop task
  evt::start();
  // event bus sniffer
  //evt::debug();

/*
  // if all 3 buttons are pressed on boot, reset EEPROM configuration to defaults
  // this does not makes much sense 'cause pressed gpio0 will put MCU into fash mode
  if (digitalRead(BUTTON_P_PIN) == LOW && digitalRead(BUTTON_N_PIN) == LOW &&
      digitalRead(BUTTON_PIN) == HIGH) {
    write_default_EEPROM();
  }
*/

  // fake readings for now
  int VoltageValue = 20;

#ifdef CONFIG_TINYUSB_MSC_ENABLED
  if (QCEnable) {
    QC.begin();
    delay(100);
    switch (VoltageValue) {
      case 0: {
        QC.set9V();
      } break;
      case 1: {
        QC.set12V();
      } break;
      case 2: {
        QC.set12V();
      } break;
      case 3: {
        QC.set20V();
      } break;
      case 4: {
        QC.set20V();
      } break;
      default:
        break;
    }
  }
#endif

  // Initialize Iron Controller
  espIron.init();

  // request configured voltage via PD trigger
  PD_Update();

  // I2C bus (for display)
  Wire.begin();
  Wire.setClock(100000);  // 400000

#ifndef DEVELOP_MODE
  // initialize heater
  heater.init();

  // initialize acceleration sensor
  accel.init();

  // init voltage sensor
  vin.init();
#endif  //DEVELOP_MODE

  // initialize HID (buttons controls and navigation, screen)
  LOGI(T_IRON, println, "Init HID");
  hid.init();

  // long beep for setup completion 安装完成时长哔哔声
  beep();
}

void loop() {
  // I do not need Arduino's loop, so this thread should be terminated
  vTaskDelete(NULL);
}

// creates a short beep on the buzzer 在蜂鸣器上创建一个短的哔哔声
void beep() {
//  if (beepEnable) {
    for (uint8_t i = 0; i < 255; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delayMicroseconds(125);
      digitalWrite(BUZZER_PIN, LOW);
      delayMicroseconds(125);
    }
//  }
}


void PD_Update() {
  // fake readings for now
  int VoltageValue = 20;
  switch (VoltageValue) {
    // 9 volts
    case 9: {
      digitalWrite(PD_CFG_0, LOW);
      digitalWrite(PD_CFG_1, LOW);
      digitalWrite(PD_CFG_2, LOW);
    } break;
    // 12 volts
    case 12: {
      digitalWrite(PD_CFG_0, LOW);
      digitalWrite(PD_CFG_1, LOW);
      digitalWrite(PD_CFG_2, HIGH);
    } break;
    // 15 volts
    case 15: {
      digitalWrite(PD_CFG_0, LOW);
      digitalWrite(PD_CFG_1, HIGH);
      digitalWrite(PD_CFG_2, HIGH);
    } break;
    // 20 volts
    case 20: {
      digitalWrite(PD_CFG_0, LOW);
      digitalWrite(PD_CFG_1, HIGH);
      digitalWrite(PD_CFG_2, LOW);
    } break;
    default:
      // start PD trigger with default 5 volts
      digitalWrite(PD_CFG_0, HIGH);
  }
/*
  if (VoltageValue == 3) {
    ledcSetup(HEATER_CHANNEL, HEATER_HIGHFREQ, HEATER_RES);
  } else {
    ledcSetup(HEATER_CHANNEL, HEATER_FREQ, HEATER_RES);
  }
*/
}

static void usbEventCallback(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
  // disable it for now
/*
  if (event_base == ARDUINO_USB_EVENTS) {
    // arduino_usb_event_data_t* data = (arduino_usb_event_data_t*)event_data;
    switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT:
        // HWSerial.println("USB PLUGGED");
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(0, 10,
                     "USB PLUGGED");  // write something to the internal memory
        u8g2.sendBuffer();            // transfer internal memory to the display
        break;
      case ARDUINO_USB_STOPPED_EVENT:
        // HWSerial.println("USB UNPLUGGED");
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(
            0, 10, "USB UNPLUGGED");  // write something to the internal memory
        u8g2.sendBuffer();            // transfer internal memory to the display
        break;
      case ARDUINO_USB_SUSPEND_EVENT:
        // HWSerial.printf("USB SUSPENDED: remote_wakeup_en: %u\n",
        // data->suspend.remote_wakeup_en);
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(
            0, 10, "USB SUSPENDED");  // write something to the internal memory
        u8g2.sendBuffer();            // transfer internal memory to the display
        break;
      case ARDUINO_USB_RESUME_EVENT:
        // HWSerial.println("USB RESUMED");
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(0, 10,
                     "USB RESUMED");  // write something to the internal memory
        u8g2.sendBuffer();            // transfer internal memory to the display
        break;

      default:
        break;
    }
  } else if (event_base == ARDUINO_FIRMWARE_MSC_EVENTS) {
    // arduino_firmware_msc_event_data_t* data =
    // (arduino_firmware_msc_event_data_t*)event_data;
    switch (event_id) {
      case ARDUINO_FIRMWARE_MSC_START_EVENT:
        // HWSerial.println("MSC Update Start");
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(
            0, 10,
            "MSC Update Start");  // write something to the internal memory
        u8g2.sendBuffer();        // transfer internal memory to the display
        break;
      case ARDUINO_FIRMWARE_MSC_WRITE_EVENT:
        // HWSerial.printf("MSC Update Write %u bytes at offset %u\n",
        // data->write.size, data->write.offset);
        //  HWSerial.print(".");
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(0, 10,
                     "MSC Updating");  // write something to the internal memory
        u8g2.sendBuffer();  // transfer internal memory to the display
        break;
      case ARDUINO_FIRMWARE_MSC_END_EVENT:
        // HWSerial.printf("\nMSC Update End: %u bytes\n", data->end.size);
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(
            0, 10, "MSC Update End");  // write something to the internal memory
        u8g2.sendBuffer();  // transfer internal memory to the display
        break;
      case ARDUINO_FIRMWARE_MSC_ERROR_EVENT:
        // HWSerial.printf("MSC Update ERROR! Progress: %u bytes\n",
        // data->error.size);
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(
            0, 10,
            "MSC Update ERROR!");  // write something to the internal memory
        u8g2.sendBuffer();         // transfer internal memory to the display
        break;
      case ARDUINO_FIRMWARE_MSC_POWER_EVENT:
        // HWSerial.printf("MSC Update Power: power: %u, start: %u, eject: %u",
        // data->power.power_condition, data->power.start,
        // data->power.load_eject);
        u8g2.clearBuffer();                  // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
        u8g2.drawStr(
            0, 10,
            "MSC Update Power");  // write something to the internal memory
        u8g2.sendBuffer();        // transfer internal memory to the display
        break;

      default:
        break;
    }
  }
*/
}

