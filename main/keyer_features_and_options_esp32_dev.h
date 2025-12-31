#ifndef ESP_CONFIG_H
#define ESP_CONFIG_H

#include "sdkconfig.h"

// compile time features and options - comment or uncomment to add or delete features
// FEATURES add more bytes to the compiled binary, OPTIONS change code behavior

//#define FEATURE_BUTTONS
//#define FEATURE_COMMAND_MODE
#define FEATURE_COMMAND_LINE_INTERFACE  // Command Line Interface functionality
#define FEATURE_MEMORIES             // on the Arduino Due, you must have FEATURE_EEPROM_E24C1024 and E24C1024 EEPROM hardware in order to compile this
#define FEATURE_MEMORY_MACROS
#define FEATURE_WINKEY_EMULATION    // disabling Automatic Software Reset is highly recommended (see documentation)
#define FEATURE_BEACON                // Go into beacon mode if paddle_left pin is LOW at boot up
#define FEATURE_BEACON_SETTING        // Go into beacon mode at boot up if EEPROM setting is enabled (\_ CLI Command)
#define FEATURE_TRAINING_COMMAND_LINE_INTERFACE
////#define FEATURE_POTENTIOMETER         // do not enable unless you have a potentiometer connected, otherwise noise will falsely trigger wpm changes
// #define FEATURE_SIDETONE_SWITCH   // adds switch control for the sidetone output. requires an external toggle switch (assigned to an arduino pin - see keyer_pin_settings.h). 
//#define FEATURE_SIDETONE_NEWTONE      // Use the NewTone library, ~1k smaller code size than the standard tone library. Uses timer1 (pins 9 or 10)  https://bitbucket.org/teckel12/arduino-new-tone/wiki/Home
#define FEATURE_SERIAL_HELP
//#define FEATURE_HELL
////// #define FEATURE_PS2_KEYBOARD        // Use a PS2 keyboard to send code - Change keyboard layout (non-US) in K3NG_PS2Keyboard.h.  Additional options below.
////// #define FEATURE_USB_KEYBOARD          // Use a USB keyboard to send code - Uncomment three lines in k3ng_keyer.ino (search for note_usb_uncomment_lines)
#define FEATURE_BT_KEYBOARD  // ESP32-WROOM-32  BT 4.2 BLE and BT Classic Bluetooth using a BT Keyboard class library
// #define FEATURE_CW_COMPUTER_KEYBOARD  // Have an Arduino Due or Leonardo act as a USB HID (Human Interface Device) keyboard and use the paddle to "type" characters on the computer -- uncomment this line in ino file: #include <Keyboard.h>
#define FEATURE_DEAD_OP_WATCHDOG
#define FEATURE_AUTOSPACE
#define FEATURE_FARNSWORTH
// #define FEATURE_DL2SBA_BANKSWITCH       // Switch memory banks feature as described here: http://dl2sba.com/index.php?option=com_content&view=article&id=131:nanokeyer&catid=15:shack&Itemid=27#english

//#define FEATURE_LCD_4BIT                // classic LCD disidefplay using 4 I/O lines
// #define FEATURE_LCD_8BIT                // classic LCD display using 8 I/O lines
// #define FEATURE_LCD_ADAFRUIT_I2C          // Adafruit I2C LCD display using MCP23017 at addr 0x20
//#define FEATURE_LCD_ADAFRUIT_BACKPACK    // Adafruit I2C LCD Backup using MCP23008 (courtesy Josiah Ritchie, KE0BLL)
//#define FEATURE_LCD_YDv1                // YourDuino I2C LCD display with old LCM 1602 V1 ic
// #define FEATURE_LCD1602_N07DH      // http://linksprite.com/wiki/index.php5?title=16_X_2_LCD_Keypad_Shield_for_Arduino
//#define FEATURE_LCD_SAINSMART_I2C


// Using TST_eSPI libary for TFT displays.  To add a new display, edits in several places are required
// IO pins assigments
// A new USer_Setup.h file in the TFT_e_SPI_Custom_Config folder
// Edit the Components/TFT_eSPI/User_Select.h to add a new #elif to point to the new User_Setup.h.  Clone the existing definitions
// InMain, near the top of the file where FEATURE_TFT_DISPLAY and FEATURE_TFT_DISPLAY are defined, add new display to the 2 lines. See example below

/// Example lines in main.cpp
/// #if defined(FEATURE_IDEASPARK_LCD) || defined(FEATURE_TFT7789_3_2inch_240x320_LCD) || defined(M5STACK_CORE2) || defined(TFT_HOSYOND_320x480_LCD)
///  #define FEATURE_TFT_DISPLAY
/// #endif
/// #if !defined(FEATURE_IDEASPARK_LCD) && !defined(FEATURE_TFT7789_3_2inch_240x320_LCD) && !defined(M5STACK_CORE2) && !defined(TFT_HOSYOND_320x480_LCD)
///  #define FEATURE_TFT_DISPLAY_NOT
/// #endif

// These are for build automation.  Precompiled files will be renamed for each display type at each build.
#define NO_DISPLAY             0
#define TEXT_I2C_4x20_LCD      1
#define TFT_1_9_IDEASPARK_LCD  2
#define TFT_3_2_DIYMALLS_LCD   3
#define M5STACK_CORE2_LCD      4
#define TFT_HOSYOND_320x480_LCD 5

// Choose one of the display types by uncommenting it.  
// If using SDKConfig (with MenuConfig too) then leave all these commented out as it wil be defined in sdkconfig.h
// Otherwise this value is passed to the project level CMakeLists.txt which in turns tells the library files which config to use, and for the main file.
//#define DISPLAY_TYPE NO_DISPLAY
//#define DISPLAY_TYPE TEXT_I2C_4x20_LCD
//#define DISPLAY_TYPE TFT_1_9_IDEASPARK_LCD   // 170x320
#define DISPLAY_TYPE TFT_3_2_DIYMALLS_LCD  // 240x320
//#define DISPLAY_TYPE M5STACK_CORE2_LCD
//#define DISPLAY_TYPE TFT_HOSYOND_320x480_LCD  // 320x480 

// *** For the TFT displays you must edit the library file TFT_eSPI/User_Setup_Select.h to point to the matching User_Setup.h located in main/TFT_e_SPI_Custom_Config 
// The Setup file is then automatically selected.  When the TFT_eSPI library is updated, it will overwrite the changes in the User_Setup_Select.h file.
// You need to restore the changes to point to the corerct User_Setup_xxx.h file for build automation to work right.

#if (DISPLAY_TYPE == TEXT_I2C_4x20_LCD) || (CONFIG_DISPLAY_TYPE_NAME == TEXT_I2C_4x20_LCD)
    //#define CONFIG_DISPLAY_TYPE TEXT_I2C_4x20_LCD
    #define FEATURE_LCD_LIQUIDCRYSTAL_I2C            // for K7MDL version on ESP32-WROOM32 using esp-idf, tested on pins 21/22 i2c pins and a 4x20 display
    #define FEATURE_STRAIGHT_KEY //This features disables memory macros on ESP32 SP5IOU 20220124
#endif
#if (DISPLAY_TYPE == TFT_1_9_IDEASPARK_LCD) || (CONFIG_DISPLAY_TYPE_NAME == TFT_1_9_IDEASPARK_LCD)
    //#define CONFIG_DISPLAY_TYPE TFT_1_9_IDEASPARK_LCD  // CONFIG_ is added to name in Project level CMakeLists.txt to control 
                                                        // TFT_eSPI User_Select.h choice of file to match this display
    #define FEATURE_IDEASPARK_LCD                   // K7MDL version on ESP32-WROOM with onboard 1.9" 320x170 color LCD graphics display, uses SPI bus  
    #define FEATURE_STRAIGHT_KEY //This features disables memory macros on ESP32 SP5IOU 20220124
    #define FEATURE_TFT_DISPLAY
#endif
#if (DISPLAY_TYPE == TFT_3_2_DIYMALLS_LCD) || (CONFIG_DISPLAY_TYPE_NAME == TFT_3_2_DIYMALLS_LCD)
    //#define CONFIG_DISPLAY_TYPE TFT_3_2_DIYMALLS_LCD
    #define FEATURE_TFT7789_3_2inch_240x320_LCD     // K7MDL version on ESP32-WROOM with onboard 3.2" DIYMalls ST7789 240x320 color LCD graphics display, uses SPI bus     
    #define FEATURE_MCP23017_EXPANDER  // Add 16 external IO pins over I2C bus
    #define FEATURE_TOUCH_DISPLAY
    #define FEATURE_STRAIGHT_KEY //This features disables memory macros on ESP32 SP5IOU 20220124
    #define FEATURE_TFT_DISPLAY
    //#define USE_TOUCH_TASK  // run check_touch_buttons event handler in a task
    //#define FEATURE_SINEWAVE_SIDETONE
    //#define FEATURE_SINEWAVE_SIDETONE_USING_TIMER_1
#endif
#if (DISPLAY_TYPE == M5STACK_CORE2_LCD) || (CONFIG_DISPLAY_TYPE_NAME == M5STACK_CORE2_LCD)
    //#define CONFIG_DISPLAY_TYPE M5STACK_CORE2_LCD
    #define FEATURE_M5STACK_CORE2     // K7MDL version on ESP32-WROOM with onboard 3.2" DIYMalls ST7789 240x320 color LCD graphics display, uses SPI bus     
    //#define FEATURE_STRAIGHT_KEY //This features disables memory macros on ESP32 SP5IOU 20220124
    #define FEATURE_TFT_DISPLAY
#endif
#if (DISPLAY_TYPE == TFT_HOSYOND_320x480_LCD) || (CONFIG_DISPLAY_TYPE_NAME == TFT_HOSYOND_320x480_LCD)
    //#define CONFIG_DISPLAY_TYPE TFT_HOSYOND_320x480_LCD
    #define FEATURE_TFT_HOSYOND_320x480_LCD     // K7MDL version on ESP32-WROOM with onboard 3.2" DIYMalls ST7789 240x320 color LCD graphics display, uses SPI bus
    ///#define FEATURE_STRAIGHT_KEY //This features disables memory macros on ESP32 SP5IOU 20220124
    #define FEATURE_TFT_DISPLAY
    #define FEATURE_TOUCH_DISPLAY  // Enable Touch features
    //#define FEATURE_MCP23017_EXPANDER  // Add 16 external IO pins over I2C bus
    ////#define USE_TASK // runs main loop in a task
    ////#define USE_TOUCH_TASK  // run check_touch_buttons event handler in a task - causes WDT timeouts on this display due to SPI bus conflicts
    #define SET_CAL  // apply cal parameters set in keyer_pin_settings_esp32_dev.h file
    //#define CAL_TOUCH  // uncomment only for calibrating the display at startup, then comment out to run normal program.
#endif

//#define FEATURE_LCD_FABO_PCF8574  // https://github.com/FaBoPlatform/FaBoLCD-PCF8574-Library
//#define FEATURE_LCD_MATHERTEL_PCF8574 // https://github.com/mathertel/LiquidCrystal_PCF8574
//#define FEATURE_LCD_I2C_FDEBRABANDER //https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
//#define FEATURE_LCD_HD44780

//#define FEATURE_CW_DECODER              // https://github.com/k3ng/k3ng_cw_keyer/wiki/385-Feature:-CW-Decoder  
//#define FEATURE_SLEEP                   // go to sleep after x minutes to conserve battery power (not compatible with Arduino DUE, may have mixed results with Mega and Mega ADK)
//#define FEATURE_LCD_BACKLIGHT_AUTO_DIM  // turn off LCD backlight and/or dim Power Indicator LED after x minutes (LED requires a PWM pin)
//#define FEATURE_ROTARY_ENCODER          // rotary encoder speed control
#define FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING
// #define FEATURE_USB_MOUSE               // Uncomment three lines in k3ng_keyer.ino (search for note_usb_uncomment_lines)
//#define FEATURE_TOUCH_PADDLE_PINS //SP5IOU 20220201 Works on ESP32_DEV platform. Uses same pins as paddles if pins are touch capable. Dissactivates normal paddles
// #define FEATURE_CAPACITIVE_PADDLE_PINS  // remove the bypass capacitors on the paddle_left and paddle_right lines when using capactive paddles
// #define FEATURE_LED_RING                // Mayhew Labs Led Ring support
#define FEATURE_ALPHABET_SEND_PRACTICE  // enables command mode S command - created by Ryan, KC2ZWM
//#define FEATURE_COMMAND_MODE_PROGRESSIVE_5_CHAR_ECHO_PRACTICE // enables command mode U
//#define FEATURE_PTT_INTERLOCK 
#define FEATURE_QLF
// #define FEATURE_EEPROM_E24C1024

#define FEATURE_DYNAMIC_DAH_TO_DIT_RATIO
#define FEATURE_PADDLE_ECHO         // you may also need to comment out line 19 in the file keyer_dependencies.h
#define FEATURE_STRAIGHT_KEY_ECHO
// #define FEATURE_AMERICAN_MORSE
// #define FEATURE_4x4_KEYPAD          // code contributed by Jack, W0XR - documentation: https://github.com/k3ng/k3ng_cw_keyer/wiki/380-Feature:-Keypad
// #define FEATURE_3x4_KEYPAD          // code contributed by Jack, W0XR - documentation: https://github.com/k3ng/k3ng_cw_keyer/wiki/380-Feature:-Keypad
//#define FEATURE_SEQUENCER
//#define FEATURE_WIFI //SP5IOU 20220129 Works only for ESP32_DEV board and family HARDWARE_ESP32_DEV
//#define FEATURE_WEB_SERVER      // Details: https://github.com/k3ng/k3ng_cw_keyer/wiki/390-Feature:-Ethernet,-Web-Server,-and-Internet-Linking
//#define FEATURE_INTERNET_LINK   // Details: https://github.com/k3ng/k3ng_cw_keyer/wiki/390-Feature:-Ethernet,-Web-Server,-and-Internet-Linking

// #define FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT     // Activate the Command Line interface on the secondary serial port
//#define OPTION_PRIMARY_SERIAL_PORT_DEFAULT_WINKEY_EMULATION  // Use when activating both FEATURE_WINKEY_EMULATION and FEATURE_COMMAND_LINE_INTERFACE 
                                                             //    simultaneously.  This will make Winkey emulation be the default at boot up; 
                                                             //    hold command button down at boot up to activate CLI mode

// #define OPTION_SUPPRESS_SERIAL_BOOT_MSG
///#define OPTION_INCLUDE_PTT_TAIL_FOR_MANUAL_SENDING
///#define OPTION_EXCLUDE_PTT_HANG_TIME_FOR_MANUAL_SENDING
/// #define OPTION_WINKEY_DISCARD_BYTES_AT_STARTUP     // if ASR is not disabled, you may need this to discard errant serial port bytes at startup
 #define OPTION_WINKEY_STRICT_EEPROM_WRITES_MAY_WEAR_OUT_EEPROM // with this activated the unit will write non-volatile settings to EEPROM when set by Winkey commands
// #define OPTION_WINKEY_SEND_WORDSPACE_AT_END_OF_BUFFER
//#define OPTION_WINKEY_STRICT_HOST_OPEN               // require an admin host open Winkey command before doing any other commands
#define OPTION_WINKEY_2_SUPPORT                      // comment out to revert to Winkey version 1 emulation
#define OPTION_WINKEY_INTERRUPTS_MEMORY_REPEAT
//#define OPTION_WINKEY_UCXLOG_9600_BAUD              // use this only with UCXLog configured for Winkey 9600 baud mode
//#define OPTION_WINKEY_2_HOST_CLOSE_NO_SERIAL_PORT_RESET  // (Required for Win-Test to function)
// #define OPTION_WINKEY_FREQUENT_STATUS_REPORT         // activate this to make Winkey emulation play better with RUMlog and RUMped
// #define OPTION_WINKEY_IGNORE_LOWERCASE               // Enable for typical K1EL Winkeyer behavior (use for SkookumLogger version 1.10.14 and prior to workaround "r" bug)
// #define OPTION_WINKEY_BLINK_PTT_ON_HOST_OPEN
// #define OPTION_WINKEY_SEND_VERSION_ON_HOST_CLOSE
// #define OPTION_WINKEY_PINCONFIG_PTT_CONTROLS_PTT_LINE  // Have Winkeyer PTT setting activate/deactivate PTT line rather than control buffered character PTT hold 
// #define OPTION_REVERSE_BUTTON_ORDER                // This is mainly for the DJ0MY NanoKeyer http://nanokeyer.wordpress.com/
///#define OPTION_PROG_MEM_TRIM_TRAILING_SPACES         // trim trailing spaces from memory when programming in command mode
///#define OPTION_DIT_PADDLE_NO_SEND_ON_MEM_RPT         // this makes dit paddle memory interruption a little smoother
#define OPTION_MORE_DISPLAY_MSGS                     // additional optional display messages - comment out to save memory
// #define OPTION_WATCHDOG_TIMER                        // this enables a four second ATmega48/88/168/328 watchdog timer; use for unattended/remote operation only
// #define OPTION_MOUSE_MOVEMENT_PADDLE               // experimental (just fooling around) - mouse movement will act like a paddle
// #define OPTION_NON_ENGLISH_EXTENSIONS  // add support for additional CW characters (i.e. À, Å, Þ, etc.)
// #define OPTION_DISPLAY_NON_ENGLISH_EXTENSIONS  // LCD display suport for non-English (NO/DK/DE) characters - Courtesy of OZ1JHM
// #define OPTION_UNKNOWN_CHARACTER_ERROR_TONE
#define OPTION_DO_NOT_SAY_HI
#define OPTION_BT_KEYBOARD_US  // for ESP32 BT keyboard
 // #define OPTION_PS2_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT // makes some non-English characters from the PS2 keyboard display correctly in the LCD display (donated by Marcin sp5iou)
// #define OPTION_PS2_KEYBOARD_RESET // reset the PS2 keyboard upon startup with 0xFF (contributed by Bill, W9BEL)
// #define OPTION_SAVE_MEMORY_NANOKEYER
///#define OPTION_CW_KEYBOARD_CAPSLOCK_BEEP
// #define OPTION_CW_KEYBOARD_ITALIAN
// #define OPTION_CW_KEYBOARD_GERMAN
// #define OPTION_CW_DECODER_GOERTZEL_AUDIO_DETECTOR  // https://github.com/k3ng/k3ng_cw_keyer/wiki/385-Feature:-CW-Decoder 
// #define OPTION_INVERT_PADDLE_PIN_LOGIC
// #define OPTION_ADVANCED_SPEED_DISPLAY //enables "nerd" speed visualization on display: wpm, cpm (char per min), duration of dit and dah in milliseconds and ratio (contributed by Giorgio, IZ2XBZ)
// #define OPTION_PROSIGN_SUPPORT    // additional prosign support for paddle and straight key echo on display, CLI, and in memory storage
// #define OPTION_RUSSIAN_LANGUAGE_SEND_CLI // Russian language CLI sending support (contributed by Павел Бирюков, UA1AQC)
#define OPTION_DO_NOT_SEND_UNKNOWN_CHAR_QUESTION
#define OPTION_CMOS_SUPER_KEYER_IAMBIC_B_TIMING_ON_BY_DEFAULT
// #define OPTION_SIDETONE_DIGITAL_OUTPUT_NO_SQUARE_WAVE
// #define FEATURE_SD_CARD_SUPPORT
// #define FEATURE_SO2R_BASE           // SO2R Box base protocol extensions
// #define FEATURE_SO2R_SWITCHES       // SO2R Box TX and RX switches
// #define FEATURE_SO2R_ANTENNA        // SO2R Box antenna selection (not fully implemented)
// #define FEATURE_COMMAND_MODE_ENHANCED_CMD_ACKNOWLEDGEMENT

// #define OPTION_DIRECT_PADDLE_PIN_READS_MEGA   // only works with Mega and pins 2 and 5 - minor performance increase
// #define OPTION_DIRECT_PADDLE_PIN_READS_UNO    // Unos or Nanos pins 2 and 5 - do not enable on a nanoKeyer, it uses different pins

// #define OPTION_WORDSWORTH_CZECH
// #define OPTION_WORDSWORTH_DEUTSCH
// #define OPTION_WORDSWORTH_NORSK

//#define OPTION_EXCLUDE_EXTENDED_CLI_COMMANDS

// #define OPTION_DFROBOT_LCD_COMMAND_BUTTONS

// #define OPTION_EXCLUDE_MILL_MODE
// #define OPTION_NO_ULTIMATIC // reduce memory usage by removing ultimatic code.

// #define OPTION_DISABLE_SERIAL_PORT_CHECKING_WHILE_SENDING_CW

// #define OPTION_PERSONALIZED_STARTUP_SCREEN        // displays a user defined string of characters on the second or fourth row of the screen during startup. 1602 display requires OPTION_DO_NOT_SAY_HI
// #define OPTION_SWAP_PADDLE_PARAMETER_CHANGE_DIRECTION        // reverses the up/down direction when using the paddles to change the wpm or sidetone frequency
#define OPTION_DISPLAY_MEMORY_CONTENTS_COMMAND_MODE
// #define OPTION_BEACON_MODE_MEMORY_REPEAT_TIME        // to space out the repeated playing of memory 1 when in beacon mode
// #define OPTION_BEACON_MODE_PTT_TAIL_TIME             // adds the ptt tail time to each playing of memory 1 in beacon mode

//  #define OPTION_WINKEY_PROSIGN_COMPATIBILITY  // Additional character mappings to support K1EL Winkey emulation prosigns

//  These are for ESP32 BT Keyboard.  THE BT keys are translated to match the PS2 key codes
// could also include PS2_keyboard.h and still work.
// Every call to read() returns a single byte for each
// keystroke.  These configure what byte will be returned
// for each "special" key.  To ignore a key, use zero.
#define PS2_TAB         9
#define PS2_ENTER       13
#define PS2_BACKSPACE   127
#define PS2_ESC         27
#define PS2_INSERT      128
#define PS2_DELETE      127
#define PS2_HOME        129
#define PS2_END         156
#define PS2_PAGEUP      25
#define PS2_PAGEDOWN    26
#define PS2_UPARROW     11
#define PS2_LEFTARROW   8
#define PS2_DOWNARROW   10
#define PS2_RIGHTARROW  21
#define PS2_F1          130
#define PS2_F2          131
#define PS2_F3          132
#define PS2_F4          133
#define PS2_F5          134
#define PS2_F6          135
#define PS2_F7          136
#define PS2_F8          137
#define PS2_F9          138
#define PS2_F10         139
#define PS2_F11         140
#define PS2_F12         141
#define PS2_SCROLL      142

#define PS2_F1_SHIFT          143
#define PS2_F2_SHIFT          144
#define PS2_F3_SHIFT          145
#define PS2_F4_SHIFT          146
#define PS2_F5_SHIFT          147
#define PS2_F6_SHIFT          148
#define PS2_F7_SHIFT          149
#define PS2_F8_SHIFT          150
#define PS2_F9_SHIFT          151
#define PS2_F10_SHIFT         152
#define PS2_F11_SHIFT         153
#define PS2_F12_SHIFT         154
#define PS2_TAB_SHIFT         9
#define PS2_ENTER_SHIFT       13
#define PS2_BACKSPACE_SHIFT   157

#define PS2_LEFT_ALT	158
//#define PS2_LEFT_CTRL	159
#define PS2_SCROLL_SHIFT 160

#if defined(OPTION_BT_KEYBOARD_US)
#define PS2_A_CTRL 162
#define PS2_B_CTRL 163
#define PS2_C_CTRL 164
#define PS2_D_CTRL 165
#define PS2_E_CTRL 166
#define PS2_F_CTRL 167
#define PS2_G_CTRL 168
#define PS2_H_CTRL 169
#define PS2_I_CTRL 170
#define PS2_J_CTRL 171
#define PS2_K_CTRL 172
#define PS2_L_CTRL 173
#define PS2_M_CTRL 174
#define PS2_N_CTRL 175
#define PS2_O_CTRL 176
#define PS2_P_CTRL 177
#define PS2_Q_CTRL 178
#define PS2_R_CTRL 179
#define PS2_S_CTRL 180
#define PS2_T_CTRL 181
#define PS2_U_CTRL 182
#define PS2_V_CTRL 183
#define PS2_W_CTRL 184
#define PS2_X_CTRL 185
#define PS2_Y_CTRL 186
#define PS2_Z_CTRL 187
#define PS2_F1_CTRL 188
#define PS2_F2_CTRL 189
#define PS2_F3_CTRL 190
#define PS2_F4_CTRL 191
#define PS2_F5_CTRL 192
#define PS2_F6_CTRL 193
#define PS2_F7_CTRL 194
#define PS2_F8_CTRL 195
#define PS2_F9_CTRL 196
#define PS2_F10_CTRL 197
#define PS2_F11_CTRL 198
#define PS2_F12_CTRL 199
#define PS2_F1_ALT 200
#define PS2_F2_ALT 201
#define PS2_F3_ALT 202
#define PS2_F4_ALT 203
#define PS2_F5_ALT 204
#define PS2_F6_ALT 205
#define PS2_F7_ALT 206
#define PS2_F8_ALT 207
#define PS2_F9_ALT 208
#define PS2_F10_ALT 209
#define PS2_F11_ALT 210
#define PS2_F12_ALT 211
#endif //OPTION_PS2_KEYBOARD_US

// ----------------- added K3NG

#if defined(OPTION_BT_KEYBOARD_GERMAN)

#define PS2_A_CTRL 162
#define PS2_B_CTRL 163
#define PS2_C_CTRL 200 //164
#define PS2_D_CTRL 165
#define PS2_E_CTRL 166
#define PS2_F_CTRL 201 //167
#define PS2_G_CTRL 168
#define PS2_H_CTRL 169
#define PS2_I_CTRL 170
#define PS2_J_CTRL 171
#define PS2_K_CTRL 172
#define PS2_L_CTRL 173
#define PS2_M_CTRL 174
#define PS2_N_CTRL 175
#define PS2_O_CTRL 202 //176
#define PS2_P_CTRL 177
#define PS2_Q_CTRL 203 //178
#define PS2_R_CTRL 204 //179
#define PS2_S_CTRL 180
#define PS2_T_CTRL 205 //181
#define PS2_U_CTRL 182
#define PS2_V_CTRL 183
#define PS2_W_CTRL 184
#define PS2_X_CTRL 185
#define PS2_Y_CTRL 186
#define PS2_Z_CTRL 187
#define PS2_F1_CTRL 188
#define PS2_F2_CTRL 189
#define PS2_F3_CTRL 190
#define PS2_F4_CTRL 191
#define PS2_F5_CTRL 192
#define PS2_F6_CTRL 193
#define PS2_F7_CTRL 194
#define PS2_F8_CTRL 195
#define PS2_F9_CTRL 206 //196
#define PS2_F10_CTRL 197
#define PS2_F11_CTRL 198
#define PS2_F12_CTRL 199
#define PS2_F1_ALT 229
#define PS2_F2_ALT 230
#define PS2_F3_ALT 231
#define PS2_F4_ALT 232
#define PS2_F5_ALT 233
#define PS2_F6_ALT 234
#define PS2_F7_ALT 235
#define PS2_F8_ALT 236
#define PS2_F9_ALT 237
#define PS2_F10_ALT 238
#define PS2_F11_ALT 239
#define PS2_F12_ALT 240

//#define PS2_INVERTED_EXCLAMATION  161 // ¡
//#define PS2_CENT_SIGN     162 // ¢
//#define PS2_POUND_SIGN      163 // £
#define PS2_CURRENCY_SIGN   164 // ¤
//#define PS2_YEN_SIGN      165 // ¥
//#define PS2_BROKEN_BAR                  166 // ¦
#define PS2_SECTION_SIGN    167 // §
//#define PS2_DIAERESIS     168 // ¨
//#define PS2_COPYRIGHT_SIGN    169 // ©
//#define PS2_FEMININE_ORDINAL    170 // ª
//#define PS2_LEFT_DOUBLE_ANGLE_QUOTE 171 // «
//#define PS2_NOT_SIGN      172 // ¬
//#define PS2_HYPHEN      173
//#define PS2_REGISTERED_SIGN   174 // ®
//#define PS2_MACRON      175 // ¯
#define PS2_DEGREE_SIGN     176 // °
//#define PS2_PLUS_MINUS_SIGN   177 // ±
#define PS2_SUPERSCRIPT_TWO   178 // ²
#define PS2_SUPERSCRIPT_THREE   179 // ³
//#define PS2_ACUTE_ACCENT    180 // ´
#define PS2_MICRO_SIGN      181 // µ
//#define PS2_PILCROW_SIGN    182 // ¶
//#define PS2_MIDDLE_DOT      183 // ·
//#define PS2_CEDILLA     184 // ¸
//#define PS2_SUPERSCRIPT_ONE   185 // ¹
//#define PS2_MASCULINE_ORDINAL   186 // º
//#define PS2_RIGHT_DOUBLE_ANGLE_QUOTE  187 // »
//#define PS2_FRACTION_ONE_QUARTER  188 // ¼
//#define PS2_FRACTION_ONE_HALF   189 // ½
//#define PS2_FRACTION_THREE_QUARTERS 190 // ¾
//#define PS2_INVERTED_QUESTION MARK  191 // ¿
//#define PS2_A_GRAVE     192 // À
//#define PS2_A_ACUTE     193 // Á
//#define PS2_A_CIRCUMFLEX    194 // Â
//#define PS2_A_TILDE     195 // Ã
#define PS2_A_DIAERESIS     196 // Ä
//#define PS2_A_RING_ABOVE    197 // Å
//#define PS2_AE        198 // Æ
//#define PS2_C_CEDILLA     199 // Ç
//#define PS2_E_GRAVE     200 // È
//#define PS2_E_ACUTE     201 // É
//#define PS2_E_CIRCUMFLEX    202 // Ê
//#define PS2_E_DIAERESIS     203 // Ë
//#define PS2_I_GRAVE     204 // Ì
//#define PS2_I_ACUTE     205 // Í
//#define PS2_I_CIRCUMFLEX    206 // Î
//#define PS2_I_DIAERESIS     207 // Ï
//#define PS2_ETH       208 // Ð
//#define PS2_N_TILDE     209 // Ñ
//#define PS2_O_GRAVE     210 // Ò
//#define PS2_O_ACUTE     211 // Ó
//#define PS2_O_CIRCUMFLEX    212 // Ô
//#define PS2_O_TILDE     213 // Õ
#define PS2_O_DIAERESIS     214 // Ö
//#define PS2_MULTIPLICATION    215 // ×
//#define PS2_O_STROKE      216 // Ø
//#define PS2_U_GRAVE     217 // Ù
//#define PS2_U_ACUTE     218 // Ú
//#define PS2_U_CIRCUMFLEX    219 // Û
#define PS2_U_DIAERESIS     220 // Ü
//#define PS2_Y_ACUTE     221 // Ý
//#define PS2_THORN     222 // Þ
#define PS2_SHARP_S     223 // ß
//#define PS2_a_GRAVE     224 // à
//#define PS2_a_ACUTE     225 // á
//#define PS2_a_CIRCUMFLEX    226 // â
//#define PS2_a_TILDE     227 // ã
#define PS2_a_DIAERESIS     228 // ä
//#define PS2_a_RING_ABOVE    229 // å
//#define PS2_ae        230 // æ
//#define PS2_c_CEDILLA     231 // ç
//#define PS2_e_GRAVE     232 // è
//#define PS2_e_ACUTE     233 // é
//#define PS2_e_CIRCUMFLEX    234 // ê
//#define PS2_e_DIAERESIS     235 // ë
//#define PS2_i_GRAVE     236 // ì
//#define PS2_i_ACUTE     237 // í
//#define PS2_i_CIRCUMFLEX    238 // î
//#define PS2_i_DIAERESIS     239 // ï
//#define PS2_eth       240 // ð
//#define PS2_n_TILDE     241 // ñ
//#define PS2_o_GRAVE     242 // ò
//#define PS2_o_ACUTE     243 // ó
//#define PS2_o_CIRCUMFLEX    244 // ô
//#define PS2_o_TILDE     245 // õ
#define PS2_o_DIAERESIS     246 // ö
//#define PS2_DIVISION      247 // ÷
//#define PS2_o_STROKE      248 // ø
//#define PS2_u_GRAVE     249 // ù
//#define PS2_u_ACUTE     250 // ú
//#define PS2_u_CIRCUMFLEX    251 // û
#define PS2_u_DIAERESIS     252 // ü
//#define PS2_y_ACUTE     253 // ý
//#define PS2_thorn     254 // þ
//#define PS2_y_DIAERESIS     255 // ÿ
#endif //defined(OPTION_PS2_KEYBOARD_GERMAN)

#if defined(OPTION_BT_KEYBOARD_FRENCH)
#define PS2_A_CTRL 162
#define PS2_B_CTRL 200 //163
#define PS2_C_CTRL 164
#define PS2_D_CTRL 165
#define PS2_E_CTRL 166
#define PS2_F_CTRL 201 //167
#define PS2_G_CTRL 202 //168
#define PS2_H_CTRL 169
#define PS2_I_CTRL 170
#define PS2_J_CTRL 171
#define PS2_K_CTRL 172
#define PS2_L_CTRL 173
#define PS2_M_CTRL 174
#define PS2_N_CTRL 175
#define PS2_O_CTRL 203 //176
#define PS2_P_CTRL 177
#define PS2_Q_CTRL 204 //178
#define PS2_R_CTRL 179
#define PS2_S_CTRL 180
#define PS2_T_CTRL 205 //181
#define PS2_U_CTRL 182
#define PS2_V_CTRL 183
#define PS2_W_CTRL 184
#define PS2_X_CTRL 185
#define PS2_Y_CTRL 186
#define PS2_Z_CTRL 187
#define PS2_F1_CTRL 188
#define PS2_F2_CTRL 189
#define PS2_F3_CTRL 190
#define PS2_F4_CTRL 191
#define PS2_F5_CTRL 192
#define PS2_F6_CTRL 193
#define PS2_F7_CTRL 194
#define PS2_F8_CTRL 195
#define PS2_F9_CTRL 196
#define PS2_F10_CTRL 197
#define PS2_F11_CTRL 198
#define PS2_F12_CTRL 199
#define PS2_F1_ALT 233
#define PS2_F2_ALT 234
#define PS2_F3_ALT 235
#define PS2_F4_ALT 236
#define PS2_F5_ALT 237
#define PS2_F6_ALT 238
#define PS2_F7_ALT 239
#define PS2_F8_ALT 240
#define PS2_F9_ALT 241
#define PS2_F10_ALT 242
#define PS2_F11_ALT 243
#define PS2_F12_ALT 244

//#define PS2_INVERTED_EXCLAMATION  161 // ¡
//#define PS2_CENT_SIGN     162 // ¢
#define PS2_POUND_SIGN      163 // £
//#define PS2_CURRENCY_SIGN   164 // ¤
//#define PS2_YEN_SIGN      165 // ¥
//#define PS2_BROKEN_BAR                  166 // ¦
#define PS2_SECTION_SIGN    167 // §
#define PS2_DIAERESIS     168 // ¨
//#define PS2_COPYRIGHT_SIGN    169 // ©
//#define PS2_FEMININE_ORDINAL    170 // ª
//#define PS2_LEFT_DOUBLE_ANGLE_QUOTE 171 // «
//#define PS2_NOT_SIGN      172 // ¬
//#define PS2_HYPHEN      173
//#define PS2_REGISTERED_SIGN   174 // ®
//#define PS2_MACRON      175 // ¯
#define PS2_DEGREE_SIGN     176 // °
//#define PS2_PLUS_MINUS_SIGN   177 // ±
#define PS2_SUPERSCRIPT_TWO   178 // ²
//#define PS2_SUPERSCRIPT_THREE   179 // ³
//#define PS2_ACUTE_ACCENT    180 // ´
#define PS2_MICRO_SIGN      181 // µ
//#define PS2_PILCROW_SIGN    182 // ¶
//#define PS2_MIDDLE_DOT      183 // ·
//#define PS2_CEDILLA     184 // ¸
//#define PS2_SUPERSCRIPT_ONE   185 // ¹
//#define PS2_MASCULINE_ORDINAL   186 // º
//#define PS2_RIGHT_DOUBLE_ANGLE_QUOTE  187 // »
//#define PS2_FRACTION_ONE_QUARTER  188 // ¼
//#define PS2_FRACTION_ONE_HALF   189 // ½
//#define PS2_FRACTION_THREE_QUARTERS 190 // ¾
//#define PS2_INVERTED_QUESTION MARK  191 // ¿
//#define PS2_A_GRAVE     192 // À
//#define PS2_A_ACUTE     193 // Á
//#define PS2_A_CIRCUMFLEX    194 // Â
//#define PS2_A_TILDE     195 // Ã
//#define PS2_A_DIAERESIS     196 // Ä
//#define PS2_A_RING_ABOVE    197 // Å
//#define PS2_AE        198 // Æ
//#define PS2_C_CEDILLA     199 // Ç
//#define PS2_E_GRAVE     200 // È
//#define PS2_E_ACUTE     201 // É
//#define PS2_E_CIRCUMFLEX    202 // Ê
//#define PS2_E_DIAERESIS     203 // Ë
//#define PS2_I_GRAVE     204 // Ì
//#define PS2_I_ACUTE     205 // Í
//#define PS2_I_CIRCUMFLEX    206 // Î
//#define PS2_I_DIAERESIS     207 // Ï
//#define PS2_ETH       208 // Ð
//#define PS2_N_TILDE     209 // Ñ
//#define PS2_O_GRAVE     210 // Ò
//#define PS2_O_ACUTE     211 // Ó
//#define PS2_O_CIRCUMFLEX    212 // Ô
//#define PS2_O_TILDE     213 // Õ
//#define PS2_O_DIAERESIS     214 // Ö
//#define PS2_MULTIPLICATION    215 // ×
//#define PS2_O_STROKE      216 // Ø
//#define PS2_U_GRAVE     217 // Ù
//#define PS2_U_ACUTE     218 // Ú
//#define PS2_U_CIRCUMFLEX    219 // Û
//#define PS2_U_DIAERESIS     220 // Ü
//#define PS2_Y_ACUTE     221 // Ý
//#define PS2_THORN     222 // Þ
//#define PS2_SHARP_S     223 // ß
#define PS2_a_GRAVE     224 // à
//#define PS2_a_ACUTE     225 // á
//#define PS2_a_CIRCUMFLEX    226 // â
//#define PS2_a_TILDE     227 // ã
//#define PS2_a_DIAERESIS     228 // ä
//#define PS2_a_RING_ABOVE    229 // å
//#define PS2_ae        230 // æ
#define PS2_c_CEDILLA     231 // ç
#define PS2_e_GRAVE     232 // è
#define PS2_e_ACUTE     233 // é
//#define PS2_e_CIRCUMFLEX    234 // ê
//#define PS2_e_DIAERESIS     235 // ë
//#define PS2_i_GRAVE     236 // ì
//#define PS2_i_ACUTE     237 // í
//#define PS2_i_CIRCUMFLEX    238 // î
//#define PS2_i_DIAERESIS     239 // ï
//#define PS2_eth       240 // ð
//#define PS2_n_TILDE     241 // ñ
//#define PS2_o_GRAVE     242 // ò
//#define PS2_o_ACUTE     243 // ó
//#define PS2_o_CIRCUMFLEX    244 // ô
//#define PS2_o_TILDE     245 // õ
//#define PS2_o_DIAERESIS     246 // ö
//#define PS2_DIVISION      247 // ÷
//#define PS2_o_STROKE      248 // ø
#define PS2_u_GRAVE     249 // ù
//#define PS2_u_ACUTE     250 // ú
//#define PS2_u_CIRCUMFLEX    251 // û
//#define PS2_u_DIAERESIS     252 // ü
//#define PS2_y_ACUTE     253 // ý
//#define PS2_thorn     254 // þ
//#define PS2_y_DIAERESIS     255 // ÿ
#endif //defined(OPTION_PS2_KEYBOARD_FRENCH)

#endif // ESP_CONFIG_H