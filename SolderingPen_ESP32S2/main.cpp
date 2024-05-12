//
#include "const.h"
#include "ironcontroller.hpp"
#include "heater.hpp"
#include "sensors.hpp"
#include "hid.hpp"
#include "main.h"
#include "log.h"

#include <QC3Control.h>
#ifdef CONFIG_TINYUSB_MSC_ENABLED
#include "FirmwareMSC.h"
#include "USB.h"
#endif


#ifdef CONFIG_TINYUSB_MSC_ENABLED
QC3Control QC(QC_DP_PIN, QC_DM_PIN);
FirmwareMSC MSC_Update;
#endif
// Iron tip heater object
TipHeater heater(HEATER_PIN, HEATER_CHANNEL, HEATER_INVERT);

// acceleration sensor
GyroSensor accel;
// input voltage sensor
VinSensor vin;

// Default value can be changed by user and stored in EEPROM
// 用户可以更改并存储在EEPROM中的默认值
uint8_t MainScrType = MAINSCREEN;
bool beepEnable = BEEP_ENABLE;
bool QCEnable = QC_ENABLE;


// Default value for T12
// T12的默认值
uint16_t CalTemp[TIPMAX][4] = {TEMP200, TEMP280, TEMP360, TEMPCHP};
char TipName[TIPMAX][TIPNAMELENGTH] = {TIPNAME};
uint8_t CurrentTip = 0;
uint8_t NumberOfTips = 1;

// Variables for pin change interrupt 引脚更改中断的变量
// those a flags for press and hold for buttons
volatile uint8_t a0, b0, c0;
volatile bool ab0;
// button press counters
volatile int count, countMin, countMax, countStep;

// Timing variables 时间变量
uint32_t buttonmillis;


// Buffer for drawUTF8
char F_Buffer[20];

// Language
uint8_t language = 0;

// Hand Side
uint8_t hand_side = 0;

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
  delay(4000);
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

  // Initialize IronController
  espIron.init();

  // request configured voltage via PD trigger
  //PD_Update();

  // I2C bus (for display)
  Wire.begin();
  Wire.setClock(100000);  // 400000

  // initialize heater
  //heater.init();

  // initialize acceleration sensor
  //accel.init();

  // intit voltage sensor
  //vin.init();

  // initialize HID (buttons controls and navigation, screen)
  LOGI(T_IRON, println, "Init HID");
  hid.init();

  // long beep for setup completion 安装完成时长哔哔声
  beep();
  //beep();
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


int32_t variance(int16_t a[]) {
  // Compute mean (average of elements)计算平均值(元素的平均值)
  int32_t sum = 0;

  for (int i = 0; i < 32; i++) sum += a[i];
  int16_t mean = (int32_t)sum / 32;
  // Compute sum squared differences with mean.计算和平方差的平均值
  int32_t sqDiff = 0;
  for (int i = 0; i < 32; i++) sqDiff += (a[i] - mean) * (a[i] - mean);
  return (int32_t)sqDiff / 32;
}

void PD_Update() {
  // fake readings for now
  int VoltageValue = 20;
  switch (VoltageValue) {
    // 9 volts
    case 0: {
      digitalWrite(PD_CFG_0, LOW);
      digitalWrite(PD_CFG_1, LOW);
      digitalWrite(PD_CFG_2, LOW);
    } break;
    // 12 volts
    case 1: {
      digitalWrite(PD_CFG_0, LOW);
      digitalWrite(PD_CFG_1, LOW);
      digitalWrite(PD_CFG_2, HIGH);
    } break;
    // 15 volts
    case 2: {
      digitalWrite(PD_CFG_0, LOW);
      digitalWrite(PD_CFG_1, HIGH);
      digitalWrite(PD_CFG_2, HIGH);
    } break;
    // 20 volts
    case 3:
    case 4: {
      digitalWrite(PD_CFG_0, LOW);
      digitalWrite(PD_CFG_1, HIGH);
      digitalWrite(PD_CFG_2, LOW);
    } break;
    default:;
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

