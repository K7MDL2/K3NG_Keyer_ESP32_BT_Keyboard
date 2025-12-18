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

             ********************************  Dec 17, 2025  K7MDL *******************************
> [!NOTE]
>As of Nov 17, 2025, the precompiled .bin files have new offset numbers to use when flashing.  The Wiki pages for the 2 flash tools have been updated as well as Flash_Tool_Readme.txt.
> 
>https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki

The Paddle and Straight Key IO pins used on the MCP23017 Port Expander, and now also the local GPIO pins, are using interrupts for better performance.  The main program simply looks at a state variable set by the interrupts eliminating IO polling time.  Raised the max WPM rate to 42WPM. The local IO pins can do better than 50WPM.  More testing to come on the max for each configuration. 

Nov 28, 2025 - Enabled touch on 5 buttons on lower unused part of the 3.2" display.  This capacitive touch screen is an ESP32-2432S032C-I clone using a GT911 touch controller. The TFT_eSPI library looks for a SPI touch controller which is used for resistive display versions such as the 3.5" which I have a build for here. It needed to be initialized as I2C bus #1 despite it using pins 21 and 22 because the touch library is using pins 33 and 32 and using I2C bus #0.  When Button #1 is pressed, a blue popup window is presented over the CW text scroll area with some test text in it.  After a few seconds, it goes away and the CW text is redrawn.  This window mechanism will be used for future features like help, menus, grid and callsign input, viewing memory contents, activating memories without a keyboard, etc.  Some of these are what a physical button can do if enabled today in the code, now can be touch.   

Additionally I now can scale the status bar for higher res screens and is now done on the 3.5" 320x480 display.  I also populated the repo here with the complete components content as used in my builds so you do not need to hunt them down. The libraries folder is not used and is a carry over from the original project and some of them may be moved to components when needed to enable certain features in the future like PS2 keyboard.  It will eventually go away.

Nov 24, 2025 - Added support for 16 pin MCP23017 I2C port expansion board.  The larger display boards have very few external IO pins.  The MCP23017 connects via I2C and gives you 16 IO ports.  The program used polling for the paddles. I replaced the polling with an interrupt routine to eliminate time consuming polling over the I2C bus.  Both Paddles and straight key work.   I was able to move the top WPM speed limit up to 40WPM, maybe more.   

Added some code to make display size scaling easier.  The 3.5" display is a 320x480.  It is set up with 30 chars per line, 5 lines, and a larger size status bar and larger font.   Deleted many .h config files for non-esp32 boards as they will never compile under esp-idf.

Nov 17, 2025 - No feature changes but much behind the scenes to setup for easy adaption to future larger or different geometry screen sizes.  Faster CW text scrolling.  Looking into using viewports (aka windows) to place current pop-up messages and eliminate the slow screen redraws.  This also paves the way for future info screens like memory contents, memory editing, and proper graphics scrolling in a window that won't affect the surrounds, can just use text wrap.  This should improve CPU perf a bit and look a lot better.  The Sidetone line is now set to active Low on request (for now).  

There are 4 screen display models in the #defines. The M5Stack does not work yet, still working on it.  The other 3 are all good.  This one display setting (DISPLAY_TYPE) is passed up to the top level (project) CMakeLists.txt and is set ot CONFIG_DISPLAY_MODEL.  That controls the library choice of User_Settings.h for each display type and copying the bin files to the right folder at the end of a compile.

Improved the startup screen sequence and text placement.  Pairing screen is hard to miss now.

Nov 10, 2025 - Updated CMakeLists.txt files (project and main) to search for DISPLAY_TYPE type in keyer_features_and_options_esp32_dev.h and include the correct #define and also add the DISPLAY_TYPE as a subfolder under precompiled_image folder.  Now the latest .bin files for each display model are copied every time I do a build for that display model.  See the new Wiki page https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Select-Display-Type-for-Build

Minor changes to clear out the line before printing a status message on the screen at startup (No BT keyboard, Pairing Code XXXXXX, etc).

             ********************************  Previous Updates *********************************

Nov 10, 2025 - Removed bt_keyboard check and main_loop from tasks.  Created bt_keyboard_read() and bt_keyboard_available() to mirror the keyboard.read() and keyboard.available() functions.  Performance appears to be good with no dropped dits or dahs up to 31WPM on the text i2c LCD.  Limits are set at 30WPM.   I also changes teh sdkconfig to use 240MHz CPU and 80Mhz with QIO for memory (vs the stock 160 and 40 DIO).

I forked the TFT_eSPI library so it can be sync'd to the original when needed. This preserves my customized User_Setup_Select.h file which points to custom User_Setup.h files in a folder under /main.  One for the 1.9" TFT, the other for the 3.2" TFT.  https://github.com/K7MDL2/TFT_eSPI

The K380 BT Classic keyboard works great.  The BLE keyboards not as well, still digging into why.  No keyboard works when compiled under Arduino yet.  Both are running arduino_esp32 3.3.3 library.   See the open issues for status.

Nov 7, 2022 - Now the same code will compile under esp-idf (v5.5.1) and Arduino IDE (2.3.6).  Just change main.cpp to main.ino.   As of today, the precompiled file set is for the text LCD i2c display and compiled under esp-idf so all 3 keyboards will work.

See https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/issues/2 for what works and what does not.  Bascially on the Rii i8+ mini keyboard works under Arduino as of today, but does reconnect.  Under esp-idf, all work, the K380s wont reconnect, the K380 and Rii i8+ do.  

Below are the 3 displays that I have setup and tested on ESP32.  For the TFT displays, you mst also edit the TFTR_eSPI library file User_Setup_Select.h to point to one of the 2 TFt setup files located in main/TFT_e_SPI_Custom_Config folder.  These are preconfigured files for each display.

    //#define FEATURE_LCD_LIQUIDCRYSTAL_I2C   // for K7MDL version on ESP32-WROOM32 using esp-idf, tested on pins 21/22 i2c pins and a 4x20 display
    // *** In Arduino IDE, for these 2 TFT displays, you must edit libraries TFT_eSPI/User_Setup_Select.h to point to the matching User_Setup.h ***
    //#define FEATURE_IDEASPARK_LCD             // K7MDL version on ESP32-WROOM with onboard 1.9" 320x170 color LCD graphics display, uses SPI bus
    #define FEATURE_TFT7789_3_2inch_240x320_LCD   // K7MDL version on ESP32-WROOM with onboard 3.2" DIYMalls ST7789 240x320 color LCD graphics display, uses SPI bus

Nov 3, 2025 - With minor code change and reconfigured pin assignments I have a 3.2" st7789 320x240 TFT capcitive touchscreen display running.  I plan to try to get touch going next.  I want to add touch buttons to the lower part of the display.  Upsizing from 1.9" to 3.2" made the same fonts 50% larger, now the same size as the text LCD.   It also magnified the gaps between letters when using a proportional font with fixed spacing.  The stock code counts columns for scrolling the CCW text area.  I switched to a fixed font for the CW text box.  You get 5 lines of 20 characters each.

<img width="1772" height="2475" alt="image" src="https://github.com/user-attachments/assets/70634f97-0844-4e28-b2f2-c513508bc318" />

Nov 2, 2025 - Now have a 1.9" Color TFT display working.  Uses st7789 and is bright and sharp.  I have it configures for 5 lines at 17 characters per line.  I added Wiki page for details on how I configured this in the project.  I took a lot of trial and error. https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Color-TFT-Display-Info

<img width="1067" height="601" alt="image" src="https://github.com/user-attachments/assets/2773d19a-17c4-4a90-83e8-cc4be92991ab" />

There is a status bar with BT connection, Pause, TX/TX, Keyer mode, grid square (hard coded for now) and WPM rate. 

<img width="447" height="252" alt="image" src="https://github.com/user-attachments/assets/2ff16fb4-11a8-4e83-a45c-eaaef0f9fdd8" />

Update Oct 30, 2025.  Now have a 4 lne by 20 column LCD working.  Display is connected on pins I2C pins 21 and 22 driving a Hitachi 44780 compatible display.  Uses the LCD class structure so can swap in different libraries for different devices.

Update: Oct 29, 2025:  This is fully functional for the features I have defined.  Tested 3 BT keyboards.  The K380s is a BLE keyboard and currently does not reconnect so it requires you to pair it each connection for now.  All keys for the equivalent PS2 keyboard are assigned.  See Wiki pages.

This requires certain settings.

There are 2 relevant bugs.  
https://github.com/espressif/esp-idf/issues/15379  - BLE-HID-Device Re-Connection not working with the esp_hid_host (unresolved)
https://github.com/espressif/esp-idf/issues/12401  - HID Host fails to connect to a BLE keyboard (solved - see details of settings required)

These config values must be changed:
   
    #define CONFIG_BT_GATTC_NOTIF_REG_MAX 64  //needed for K380s BLE keyboard 
    #define CONFIG_BT_SMP_MAX_BONDS 40
    #define BTA_GATTC_CONN_MAX  64

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
