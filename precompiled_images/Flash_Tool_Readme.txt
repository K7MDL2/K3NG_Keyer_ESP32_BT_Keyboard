Flash Download Tool image files

This file set is tested on a ESP32-WROOM-32 clone board.

To flash firmware to your ESP32 board, follow the instructions located on the project GitHub site Wiki pages.

https://github.com/K7MDL2/K3NG_Keyer_ESP32_BT_Keyboard/wiki/Using-the-Flash-Download-Tool


There are 3 files plus this readme file.  The filenames are listed below along with the offset number that you will enter with it in the Flash Download Tool UI.

1. Bootloader.bin   0x1000 (1 plus 3 zeros)
2. K3NG_keyer.bin  0x10000  (2 plus 4 zeros)
3. partition-table.bin 0x8000 (9 plus 3 zeros)


Enter the above data into the tool, select your com port connected to your CPU, then hit START.  It runs fairly quick.  
Allow some boot time.  Should start responding to keystrokes.  

You can use a terminal program to connect to the CPU at 115200 baud and see the test you typed and other startup info such as BLE and BT devices discovered.