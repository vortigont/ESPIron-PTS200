/*
    This file is a part of ESPIron-PTS200 project
    https://github.com/vortigont/ESPIron-PTS200

    Copyright © 2024 Emil Muratov (vortigont)

    ESPIron-PTS200 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#pragma once

#ifdef ARDUINO_USB_MODE

#if ARDUINO_USB_MODE == 1
#warning This code intended for TinyUSB/OTG mode (ARDUINO_USB_MODE == 0)
#else
#warning Using USB in TinyUSB/OTG mode
#endif  // ARDUINO_USB_MODE == 1


#ifdef CONFIG_TINYUSB_MSC_ENABLED
#warning Building with FirmwareMSC enabled
#include "USB.h"
#include "FirmwareMSC.h"
#endif  // CONFIG_TINYUSB_MSC_ENABLED

//FirmwareMSC MSC_Update;
#endif  // ARDUINO_USB_MODE
