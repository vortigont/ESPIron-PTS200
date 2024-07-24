## Flashing guide

### Flashing and QC trigger
**!WARNING!** Acticated QC-trigger iterferes with USB data transfer. When QC is activated __no other__ USB operation is possible, including firmware download. Disable QC trigger and replug the Iron before doing firmware upgrade!

### Unbrick PTS-200v2 iron
If, for some reason, you've bricked your iron or it reboot cyclically (i.e. it is no longer able to flash using automated mode via USB), you need to bring it to flash mode manually.

 - Build your firmware using Platformio
 - Prepare to run flash command in console `pio run -t upload` but do not run it yet.
 - Take your Iron, unplug it from PC USB port, press and hold middle button, while holding the button plug-in the Iron into PC USB port
 - Hold the USB plug in your hand and run command in console `pio run -t upload`
 - Platformio will find the iron and print mesasge like this

```
CURRENT: upload_protocol = esptool
Looking for upload port...

Auto-detected: /dev/ttyACM0
Forcing reset using 1200bps open/close on port /dev/ttyACM0
Waiting for the new upload port...
``

 - immidiately when you see the message `Waiting for the new upload port...` quickly unplug and replug USB cable to your iron while STILL HOLDING MIDDLE BUTTON
 - You must see a message like this in console `Uploading .pio/build/mypen_qc/firmware.bin`
 - esptool will upload the firmware to the iron
 - after upload is done unplug and replug the Iron to quit manual flash mode


### MSC Upgrade
Please use the user version for regular upgrades. Connect the device to the computer with a USB cable, open the firmware upgrade option in the device's menu, and drag the firmware file into the device's MSC storage.

If there is an upgrade failure or other problems with the firmware, you can use the factory version of the firmware and use the ESP flash tool to write the firmware at address 0x0000. Select ESP32S2-Develop-Internal USB to download.
