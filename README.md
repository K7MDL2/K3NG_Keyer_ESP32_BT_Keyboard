# k3ng_cw_keyer
K3NG Arduino CW Keyer

The K3NG Keyer is an open source Arduino based CW (Morse Code) keyer with a lot of features and flexibility, rivaling commercial keyers which often cost significantly more. The code can be used with a full blown Arduino board or an AVR microcontroller chip can be programmed and used directly in a circuit. This keyer is suitable as a standalone keyer or for use permanently installed inside a rig, especially homebrew QRP rigs. It’s open source code so you can fully customize it to fit your needs and also perhaps learn from it or find coding ideas for other projects.

Documentation is located here:
https://github.com/k3ng/k3ng_cw_keyer/wiki



***********************  Oct 4, 2025  K7MDL *****************************

This repository is forked from https://github.com/aimeiz/k3ng_cw_keyer-master_2022 repository which was modified 3 years ago to work on some flavor of ESP32.  After changing the pin assignments it workes on my ESP32-WROOM-32 dev board.  

I then integrated a BT Keyboard library from https://github.com/turgu1/bt-keyboard.  It is a bit different in that the ESP32 is a HID host connecting to a BT keyboard for input.  Most examples just convert a USB or PS2 keyboard to BT to connect to a PC.

The K3NG USB and PS2 keyboards map a ton of functions, I plan to replicate those key mappings later.  For now I have coded all regular keys and their SHIFT+key codes also work.

Eventually I will merge these changes into the current K3NG repo and do a Pull Request there.

WiIl be adding WIKI pages to show how I set up the IDE Tool options and placing the bt_keyboad library in the right place.

Tested with BLE keyboard Rii i8+ mini keyboard.   Started testing Logitech K380 whci happears to use BT classic and is crashing during BT discovery during initial pairing, investigating....

*************************************************************************
