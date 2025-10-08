| Support Targets | ![alt text][esp32] |
| --- | --- |

| Boards Used | ![alt text][esp32-WROOM-32]|
| --- | --- |

| Dev Environment Used | ![alt text][ESP-IDF]|
| --- | --- |


[esp32]: https://img.shields.io/badge/ESP32-green "ESP32"
[esp32-WROOM-32]: https://img.shields.io/badge/ESP32--WROOM--32-orange "ESP32-WROOM-32"
[ESP-IDF]: https://img.shields.io/badge/ESP--IDF-cyan "ESP-IDF"

# k3ng_cw_keyer
K3NG Arduino CW Keyer

The K3NG Keyer is an open source Arduino based CW (Morse Code) keyer with a lot of features and flexibility, rivaling commercial keyers which often cost significantly more. The code can be used with a full blown Arduino board or an AVR microcontroller chip can be programmed and used directly in a circuit. This keyer is suitable as a standalone keyer or for use permanently installed inside a rig, especially homebrew QRP rigs. It’s open source code so you can fully customize it to fit your needs and also perhaps learn from it or find coding ideas for other projects.

Documentation is located here:
https://github.com/k3ng/k3ng_cw_keyer/wiki



***********************  Oct 4, 2025  K7MDL *****************************

This repository is forked from https://github.com/aimeiz/k3ng_cw_keyer-master_2022 repository which was modified 3 years ago by SP5IOU to work on some flavor of ESP32.  After changing the pin assignments it worked on my ESP32-WROOM-32 dev board.  

I then integrated a BT Keyboard library from https://github.com/turgu1/bt-keyboard.  It is a bit different in that the ESP32 is a HID host connecting to a BT keyboard for input.  Most examples just convert a USB or PS2 keyboard to BT to connect to a PC.   

I am using ESP-IDF to compile.  Arduino32 is added as a component.  I first tried Arduino IDE but I was not able to get a BT classic keyboard (Logitech K380) to fully connect after it was discovered.  My BLE Rii i8+ keyboard worked fine though.  My BT_Keyboard test programs behaved the same.   I ported it to esp-idf and BT classic works proper.

I was getting RTOS WDT warnings while dits and dahs were being sent, competing with the BT service task.   I put the main program loop and check_bt_keyboard() into their own RTOS tasks and the problem seems to be solved for now. Seem OK at 30WPM and 13WPM.  More testing required.

The K3NG USB and PS2 keyboards features map a ton of keys to functions, I plan to replicate those key mappings later.  For now I have coded all regular keys and their SHIFT+key codes also.

Tested with BLE keyboard Rii i8+ mini keyboard and the Logitech K380 which happens to use BT classic.

Eventually I will merge these changes into the current K3NG repo and do a Pull Request there.

Will be adding more WIKI pages here to show how I set up the IDE Tool options and placing the bt_keyboad library in the right place.  

I do have Wiki pages to show how to upload precompiled firmware to your compatible CPU board either of 2 tools so you do not have to compile the code.

https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki


*************************************************************************
