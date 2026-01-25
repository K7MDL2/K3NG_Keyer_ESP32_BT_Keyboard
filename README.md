| Support Targets | ![alt text][esp32] | ![alt text][Has_Precompiled_Firmware_Images] |
| --- | --- | --- |

| Boards Used | ![alt text][ESP32-WROOM]| ![alt text][ESP32-WROOM_with_st7789_Color_TFT_display]| ![alt text][ESP32-WROOM_with_st7796_Color_TFT_display_with_Touch]|
| --- | --- | --- | --- |

| Dev Environment Used | ![alt text][ESP-IDF]|
| --- | --- |

| BT Keyboards Tested | ![alt text][K380]|![alt text][K380s]|![alt text][Rii_mini]|
| --- | --- | --- | --- |

[ESP32]: https://img.shields.io/badge/ESP32-green "ESP32"
[ESP32-WROOM]: https://img.shields.io/badge/ESP32--WROOM-orange "ESP32-WROOM"
[ESP32-WROOM_with_st7789_Color_TFT_display]: https://img.shields.io/badge/ESP32--WROOM--with--st7789--Color--TFT--display-orange "ESP32-WROOM-TFT"
[ESP32-WROOM_with_st7796_Color_TFT_display_with_Touch]: https://img.shields.io/badge/ESP32--WROOM--with--st7796--Color--TFT--display--with--Touch-cyan "ESP32-WROOM-TFT"
[ESP-IDF]: https://img.shields.io/badge/ESP--IDF--v5.5.1-cyan "ESP-IDF v5.5.1"
[Has_Precompiled_Firmware_Images]: https://img.shields.io/badge/Has_Precompiled_Firmware_Images-purple "Precompiled_Images"
[K380]: https://img.shields.io/badge/K380-violet "K380"
[K380s]: https://img.shields.io/badge/K380s-violet "K380s"
[Rii_mini]: https://img.shields.io/badge/Rii_mini-violet "Rii i8+"


# K3NG CW Keyer (Modified for ESP32)

Based on a 2022 version modifed to run on ESP32-WROOM32.  Select updates since 2025 have been applied.  Added BT keyboards, TFT and LCD displays. Compiles under ESP-IDF, or with some limitations Arduino.
To build under ESP-IDF (preferred) see https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Building-the-Project-with-ESP%E2%80%90IDF
To build under Arduino IDE see https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Building-the-project-with-Arduino-IDE

From the orignal K3NG site:

The K3NG Keyer is an open source Arduino based CW (Morse Code) keyer with a lot of features and flexibility, rivaling commercial keyers which often cost significantly more. The code can be used with a full blown Arduino board or an AVR microcontroller chip can be programmed and used directly in a circuit. This keyer is suitable as a standalone keyer or for use permanently installed inside a rig, especially homebrew QRP rigs. It’s open source code so you can fully customize it to fit your needs and also perhaps learn from it or find coding ideas for other projects.

> [!NOTE]
>Original K3NG CW Keyer here: https://github.com/k3ng/k3ng_cw_keyer
> 
>Documentation is located here: https://github.com/k3ng/k3ng_cw_keyer/wiki
_____________________________________________

This fork on a ESP32-WROOM32 adds 
 - BT keyboards (Select BLE and BT Classic models)
 - A few different type TFT displays with 320x170, 320,240, and 480x320 resolutions
 - Touch buttons on select touch enabled displays
 - Status Bar with (future) Grid, call, WPM rate, and sevceral satus icons including T/R status.
 - MCP23017 I2C port expander to add IO to the IO starved TFT display boards.  The Paddles and Straight key are on PA0-2 pins.
 - Using interrupts for all paddle and straight key IO pin, both the port expander and local GPIO pins.

Check out the latest changes on this Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Change-Notes

> [!NOTE]
> Arduino IDE Users see https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Building-the-project-with-Arduino-IDE
>

See the Wiki Pages for more information about parts supported and configuration. https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki

This is a 3.5" 480x320 TFT with a single panel of 16 keys.  If you press the Mem key, a blue message window pops up diplaying the contents of each memory.  You can use Insert Memory macro '\Ix' to insert memories (1-9 only).  For example I have memeories 7 and 8 programmed with my callsign and grid square.

<img width="762" height="433" alt="image" src="https://github.com/user-attachments/assets/6fe568ab-11aa-40ff-be7d-16ad6caa34ff" />

This is a 3.2" 320x240 TFT with 4 rows of 4 keys (not counting the Fx key to change rows).  Has the identical 16 key assignments.

<img width="1146" height="610" alt="image" src="https://github.com/user-attachments/assets/b4512298-544b-4bc3-94d5-6bd83fdc7d14" />

Also seen in the above picture is a 16 port expander (MCP23017) on a I2C bus.  The paddles and straight key are on PA0-3.

*************************************************************************

3 years ago (2022) SP5IOU modied the K3NG Keyer Arduino code to support an ESP32.  This repository is forked from his repository at https://github.com/aimeiz/k3ng_cw_keyer-master_2022.  After changing the pin assignments it worked on my ESP32-WROOM-32 dev board.  I used the board described in this Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/CPU-Module

I then integrated a BT Keyboard library from https://github.com/turgu1/bt-keyboard.  It is a bit different in that the ESP32 is a HID host connecting to a BT keyboard for input.  Most examples just convert a USB or PS2 keyboard to BT to connect to a PC.  Later I added several types of TFT displays and touch buttons for certain models.

I am using ESP-IDF to compile.  Arduino32 is added as a component.  I first tried Arduino IDE but I was not able to get a BT classic keyboard (Logitech K380) to fully connect after it was discovered.  My BLE Rii i8+ keyboard worked fine though.  My BT_Keyboard test programs behaved the same.   I ported it to esp-idf and BT classic works proper as does the Rii. The K380s BLE keyboard works but does not reconnect and required re-pairing after each disocnnect/powerup/reset.  You can compile this under Arduino IDE but see this page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Building-the-project-with-Arduino-IDE for current functionality restrictions.

The BT keyboard translates BT key codes to match the PS2 keycodes and calls into the slightly modified PS2 keyboard function.  See the K3NG docs for USB\PS2 Keyboard commands. I have a copy of the BT keyboard commands on this Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/BT-Keyboard-Key-Assignments.   Not all keys on a PS2/USB keyboard are present on these compact BT keyboards.   I may alter some of the key assignments over time.

Tested with BLE K380s and Rii i8+ mini keyboards, and the Logitech K380, which happens to use BT classic.

I plan to fork the original K3NG repo then merge my changes into it so that this benefit from the updates to the original.  Since this version is currently compiled under esp-idf framework, it is not likely to be accepted into the main repo.  - you can compile under Arduino but all BT keyboards do not work yet.

I have several WIKI pages to show how I set up the IDE Tool options.  The TFT_eSPI and BT_keyboard files are included as a component so there should be no need to find and download libraries and components unlike in Arduino.

I provide precompiled firmware files for your compatible CPU board using either of 2 tools so you do not have to compile the code, just upload the binary files and run it. There are Wiki pages about hardware, other info, and how to upload the firmware: 

https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki

- Mike K7MDL

********************************
*****************************************
