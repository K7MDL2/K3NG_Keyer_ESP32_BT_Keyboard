| Support Targets | ![alt text][esp32] | ![alt text][Has_Precompiled_Firmware_Images] |
| --- | --- | --- |

| Boards Used | ![alt text][ESP32-WROOM]| ![alt text][ESP32-WROOM_with_st7789_Color_TFT_display]| ![alt text][ESP32-WROOM_with_st7796_Color_TFT_display]|
| --- | --- | --- | --- |

| Dev Environment Used | ![alt text][ESP-IDF]|
| --- | --- |

| BT Keyboards Tested | ![alt text][K380]|![alt text][K380s]|![alt text][Rii_mini]|
| --- | --- | --- | --- |

[ESP32]: https://img.shields.io/badge/ESP32-green "ESP32"
[ESP32-WROOM]: https://img.shields.io/badge/ESP32--WROOM-orange "ESP32-WROOM"
[ESP32-WROOM_with_st7789_Color_TFT_display]: https://img.shields.io/badge/ESP32--WROOM--with--st7789--Color--TFT--display-orange "ESP32-WROOM-TFT"
[ESP32-WROOM_with_st7796_Color_TFT_display]: https://img.shields.io/badge/ESP32--WROOM--with--st7796--Color--TFT--display-orange "ESP32-WROOM-TFT"
[ESP-IDF]: https://img.shields.io/badge/ESP--IDF--v5.5.1-cyan "ESP-IDF v5.5.1"
[Has_Precompiled_Firmware_Images]: https://img.shields.io/badge/Has_Precompiled_Firmware_Images-purple "Precompiled_Images"
[K380]: https://img.shields.io/badge/K380-violet "K380"
[K380s]: https://img.shields.io/badge/K380s-violet "K380s"
[Rii_mini]: https://img.shields.io/badge/Rii_mini-violet "Rii i8+"


# K3NG CW Keyer (Modified for ESP32)

Based on a 2022 version, modifed to run on ESP32-WROOM32 with BT keyboards and TFT and LCD displays

The K3NG Keyer is an open source Arduino based CW (Morse Code) keyer with a lot of features and flexibility, rivaling commercial keyers which often cost significantly more. The code can be used with a full blown Arduino board or an AVR microcontroller chip can be programmed and used directly in a circuit. This keyer is suitable as a standalone keyer or for use permanently installed inside a rig, especially homebrew QRP rigs. It’s open source code so you can fully customize it to fit your needs and also perhaps learn from it or find coding ideas for other projects.

> [!NOTE]
>Original K3NG CW Keyer here: https://github.com/k3ng/k3ng_cw_keyer
> 
>Documentation is located here: https://github.com/k3ng/k3ng_cw_keyer/wiki
_____________________________________________

This fork on a ESP32-WROOM32 adds BT keyboard (BLE and BT Classic) and a few different type TFT displays including touch buttons in some cases.  Below are the more recent change notes.  See the Wiki Pages for more information about parts suppotted and configuration.

             ********************************  Dec 17, 2025  K7MDL *******************************
> [!NOTE]
>As of Nov 17, 2025, the precompiled .bin files have new offset numbers to use when flashing.  The Wiki pages for the 2 flash tools have been updated as well as Flash_Tool_Readme.txt.
> 
>https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki

The Paddle and Straight Key IO pins used on the MCP23017 Port Expander, and now also the local GPIO pins, are using interrupts for better performance.  The main program simply looks at a state variable set by the interrupts eliminating IO polling time.  Raised the max WPM rate to 42WPM. The local IO pins can do better than 50WPM.  More testing to come on the max for each configuration. 


*************************************************************************

3 years ago (2022) SP5IOU modied the K3NG Keyer Arduino code to support an ESP32.  This repository is forked from his repository at https://github.com/aimeiz/k3ng_cw_keyer-master_2022.  After changing the pin assignments it worked on my ESP32-WROOM-32 dev board.  I used the board described in this Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/CPU-Module

I then integrated a BT Keyboard library from https://github.com/turgu1/bt-keyboard.  It is a bit different in that the ESP32 is a HID host connecting to a BT keyboard for input.  Most examples just convert a USB or PS2 keyboard to BT to connect to a PC.   

I am using ESP-IDF to compile.  Arduino32 is added as a component.  I first tried Arduino IDE but I was not able to get a BT classic keyboard (Logitech K380) to fully connect after it was discovered.  My BLE Rii i8+ keyboard worked fine though.  My BT_Keyboard test programs behaved the same.   I ported it to esp-idf and BT classic works proper as does the Rii. The K380s BLE keyboard does not reconnect.

I was getting RTOS WDT warnings while dits and dahs were being sent, competing with the BT service task.   I put the main program loop and check_bt_keyboard() into their own RTOS tasks and the problem seems to be solved for now. Seem OK at 30WPM and 13WPM.  More testing required.

The BT keyboard translates BT key codes to match the PS2 keycodes and calls into the slightly modified PS2 keyboard function.  See the K3NG docs for USB\PS2 Keyboard commands. I have a copy of the BT keyboard commands on this Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/BT-Keyboard-Key-Assignments.   Not all keys on a PS2/USB keyboard are present on these compact BT keyboards.   I may alter some of the key assignments over time.

Tested with BLE K380s and Rii i8+ mini keyboards, and the Logitech K380 which happens to use BT classic.

I plan to fork the original K2NG repo then merge my changes into it so that this benefit from the updates to the original.  Since this version is currently compiled under esp-idf framework, it is not likely to be accepted into the main repo.  - As of 11/6/2025, can compile under Arduino but 2 of 3 keyboards do nto work yet.

I plan to add more WIKI pages here to show how I set up the IDE Tool options.  The BT_keyboard files are included as a component so there should be no need to find and download libraries and components unlike in Arduino.  When someone does a clean install and compile this can be verified.

I provide precompiled firmware using the 4x20 line classic LCD display for your compatible CPU board using either of 2 tools so you do not have to compile the code, just upload the binary files and run it. There are Wiki pages about hardware, other info, and how to upload the firmware: 

https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki

- Mike K7MDL

********************************
*****************************************
