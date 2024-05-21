[![PlatformIO CI](https://github.com/vortigont/ESPIron-PTS200/actions/workflows/pio_build.yml/badge.svg)](https://github.com/vortigont/ESPIron-PTS200/actions/workflows/pio_build.yml)

# ESPIron PTS200

Firmware for [Feizer PTS200](https://www.aliexpress.com/store/1102411999) soldering iron.

I've started this project as a fork of [Songguo PTS200](https://github.com/Eddddddddy/Songguo-PTS200) to completely rework original firmware.
My goals is to create a well-structured firmware with a flexible design.

Currently project is in it's early development stage. Additional features and UI redesign will follow.

Discussion forum [thread](https://community.alexgyver.ru/threads/proshivka-dlja-pajalnika-feizer-pts200-v2-esp32.9930/) (in russian).

Telegram group [LampDevs](https://t.me/LampDevs) (Russian)

WIP:
 - decomposite project into independent tasks and RTOS threads
 - use full-range PWM for heater
 - enabled PID control for PWM (not optimized yet)
 - revised temperture probes scheduling
 - exclude useless tight loops
 - removed ALL blocking code, wiped arduino's `loop()` hooks, now all code is asynchronous
 - rework UI into non-blocking event based configuration system
 - Move configuration data to NVS key/value storage
 - Reimplemented temperature and timers configuration


### Key controls
| Mode | Key | Action | Function |
|-|-|-|-|
| Idle | `middle` | single click | Toggle heating mode on/off |
| Suspended | `middle` | single click | WakeUp |
| Working | `middle` | double click | Boost on/off |
| Idle/Working | `middle` | long press | Enter configuration menu |
| Working | `+`/`-` | single click | inc/decr working temperature in one step |
| Working | `+`/`-` | double click | inc/decr working temperature in 4 steps |
| Working | `+`/`-` | triple click | inc/decr working temperature in 6 steps |
| Menu navigation | `middle` | short press | enter menu section, comfirm action |
| Menu navigation | `middle` | long press | escape action, previous page, etc |
| Menu navigation | `+`/`-` | single click | move along menu items, inc/decr action  |



### Menu navigation

<!-- https://stackoverflow.com/questions/37349314/is-it-possible-to-add-border-to-image-in-github-markdown -->
Menu navigation demo capture.
|<img src="pics/menu_demo01.png?raw=true" alt="Menu demo" />
|-


### Features implementation progress

| Function | Status |
|-|-|
| standby timer | :white_check_mark: done |
| :hourglass: idle timer | :white_check_mark: done |
| :zzz: suspend timer | :white_check_mark: done |
| :hotsprings: Temperature control | :white_check_mark: done |
| :rocket: boost mode | :white_check_mark: done |
| :floppy_disk: (optionaly) save/restore last used temperature | :white_check_mark: done |
| Tip calibration | :x: Planned |
| Tip profiles | :x: Planned |
| PD configuration | :x: Planned |
| QC Configuration | :x: Planned |
| Power profile | :x: Planned |
| Power budget | :x: Planned |
| PID profiles | :x: Planned |
| PID tuning | :x: Planned |




#### Schematics
Iron [Schematics](/docs/PTS200_Schematic_2022-07-10.pdf) (probably from similar iron, some items does not match with PTS-200)




## Introduction
1. PD3.0 and QC3 fast charge protocol

2. 20V 5A 100W maximum power
<!-- 内置IMU，用于休眠检测 -->
3. Built-in IMU for sleep detection
<!-- PD协议芯片使用CH224K -->
4. PD protocol chip uses CH224K
<!-- MOSFET支持30V 12A -->
5. MOSFET supports 30V 12A
<!-- MCU使用ESP32 S2 FH4 -->
6. MCU uses ESP32 S2 FH4
<!-- 电源输入使用功率加强的USB-C接口 -->
7. The power input uses a power-enhanced USB-C interface
<!-- 定制的4欧姆内阻的烙铁头 -->
8. Customized soldering tip with 4 ohm internal resistance. It can be powered by 20V with 100W.
<!-- 128x64 OLED screen -->
9. 128x64 OLED screen
<!-- 3个按键，中间的按键与GPIO0相连 -->
10. 3 buttons, the middle button is connected to GPIO0
<!-- MSC 模式的固件升级，闪存盘模式 -->
11. MSC firmware upgrade, flash disk mode
<!-- 带有便携式的尖端保护盖 -->
12. With a portable tip cap

<!-- 构建方法 -->
## Build method
Use [PlatformIO](https://platformio.org/) to build the project.
Attach the iron to USB port and run `pio run -t upload` in project's directory.
