#ifndef ESP_CONFIG_H
#define ESP_CONFIG_H

#if !defined(PROJECT_ESP32_COMPILER)
//#include "../build/config/sdkconfig.h"
#endif

#define CODE_VERSION "K7MDL-2026.2.15"
#define eeprom_magic_number 33          // you can change this number to have the unit re-initialize EEPROM

// compile time features and options - comment or uncomment to add or delete features
// FEATURES add more bytes to the compiled binary, OPTIONS change code behavior

//  Use empty string ("") if there is no GPS active.  No grid will display on status row.
//#define FEATURE_BUTTONS
//#define FEATURE_COMMAND_MODE
#define DECLINATION  (0.0)  // decimal degrees.minutes  Ex: 14.83 is East +14 degrees 58 minutes converted to decimal
#define FEATURE_COMMAND_LINE_INTERFACE  // Command Line Interface functionality
#define FEATURE_MEMORIES             // on the Arduino Due, you must have FEATURE_EEPROM_E24C1024 and E24C1024 EEPROM hardware in order to compile this
#define FEATURE_MEMORY_MACROS
#define FEATURE_WINKEY_EMULATION    // disabling Automatic Software Reset is highly recommended (see documentation)
#define FEATURE_BEACON                // Go into beacon mode if paddle_left pin is LOW at boot up
#define FEATURE_BEACON_SETTING        // Go into beacon mode at boot up if EEPROM setting is enabled (\_ CLI Command)
//#define FEATURE_TRAINING_COMMAND_LINE_INTERFACE
////#define FEATURE_POTENTIOMETER         // do not enable unless you have a potentiometer connected, otherwise noise will falsely trigger wpm changes
// #define FEATURE_SIDETONE_SWITCH   // adds switch control for the sidetone output. requires an external toggle switch (assigned to an arduino pin - see keyer_pin_settings.h). 
//#define FEATURE_SIDETONE_NEWTONE      // Use the NewTone library, ~1k smaller code size than the standard tone library. Uses timer1 (pins 9 or 10)  https://bitbucket.org/teckel12/arduino-new-tone/wiki/Home
#define FEATURE_SERIAL_HELP
//#define FEATURE_HELL
////#define FEATURE_PS2_KEYBOARD        // Use a PS2 keyboard to send code - Change keyboard layout (non-US) in K3NG_PS2Keyboard.h.  Additional options below.
////#define FEATURE_USB_KEYBOARD          // Use a USB keyboard to send code - Uncomment three lines in k3ng_keyer.ino (search for note_usb_uncomment_lines)

#define FEATURE_BT_KEYBOARD  // ESP32-WROOM-32  BT 4.2 BLE and BT Classic Bluetooth using a BT Keyboard class library, PICO2W BLE and BT Classic

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

// These are for build automation.  Precompiled files will be renamed for each display type at each build.
#define NO_DISPLAY               0
#define TFT_320x480_CAP_LCD      1
#define TFT_PICO_320x480_RES_LCD 2

// Choose one of the display types by uncommenting it.  
// If using SDKConfig (with MenuConfig too) then leave all these commented out as it wil be defined in sdkconfig.h
// Otherwise this value is passed to the project level CMakeLists.txt which in turns tells the library files which config to use, and for the main file.

//#define DISPLAY_TYPE NO_DISPLAY
//#define DISPLAY_TYPE TFT_320x480_CAP_LCD  // Sparkle or DIYMalls 3.5" ST7796 320x480 with GT911 capacitive touch
#define DISPLAY_TYPE TFT_PICO_320x480_RES_LCD  // 3.5" 320x480 Waveshare with XPT2046 resistive Touch controller, IL:9488 display, usng TFT_eSPI lib

// *** For the TFT displays you must edit the library file TFT_eSPI/User_Setup_Select.h to point to the matching User_Setup.h located in main/TFT_e_SPI_Custom_Config 
// The Setup file is then automatically selected.  When the TFT_eSPI library is updated, it will overwrite the changes in the User_Setup_Select.h file.
// You need to restore the changes to point to the corerct User_Setup_xxx.h file for build automation to work right.

#if (DISPLAY_TYPE == TFT_320x480_CAP_LCD) || (CONFIG_DISPLAY_TYPE_NAME == TFT_320x480_CAP_LCD)
    //#define CONFIG_DISPLAY_TYPE TFT_320x480_CAP_LCD
    #define FEATURE_TFT_320x480_CAP_LCD    // K7MDL version on ESP32-WROOM with onboard 3.2" DIYMalls ST7789 240x320 color LCD graphics display, uses SPI bus
    //#define FEATURE_STRAIGHT_KEY    //This features disables memory macros on ESP32 SP5IOU 20220124 - ?? Need to verify.  /I works.
    #define FEATURE_TFT_DISPLAY
    #define TFT_320_480
    #define TOUCH_BUTTON_16
    #define FEATURE_TOUCH_DISPLAY  // Enable Touch features
    #define TOUCH_GT911_BUTTONS // use GT911 touch controller for buttons
    #define TOUCH_BUTTON_16
    //#define FEATURE_MCP23017_EXPANDER  // Add 16 external IO pins over I2C bus paddles and key on PA0-2
    //#define FEATURE_COMPASS  // read magnetic compass and temperature on a GPS https://www.amazon.com/dp/B08NY9JSZ3
    #define FEATURE_GPS  // if enabled and not GPS, use DEFAULT_GRID = "" and supply memory 9 with a grid manually.
    #define GPS_BAUD_RATE 9600    // for the hardware serial port for GPS connection if used.
    #define GPS_SERIAL_INVERT 0   // invert the RX_pin signal if needed.  Common if connecting without buffers.
    //#define GPS_TEST  // uses nmea.h to provide simulated NMEA for CN87 or EM10 grids   
    #define DEFAULT_GRID "CN88ss"  // substitute for GPS supplied grid square. This and GPS time will display on status row.
    //#define USE_TOUCH_TASK  // run check_touch_buttons event handler in a task
    #define MAX_TX_PORTS 2  // nunmber of transmitter lines (TX) for TX Select touch button
#endif

#if (DISPLAY_TYPE == TFT_PICO_320x480_RES_LCD) || (CONFIG_DISPLAY_TYPE_NAME == TFT_PICO_320x480_RES_LCD)
    //#define CONFIG_DISPLAY_TYPE  TFT_PICO_320x480_RES_LCD
    #define FEATURE_TFT_PICO_320x480_RES_LCD     // K7MDL version on ESP32-WROOM with onboard 3.2" DIYMalls ST7789 240x320 color LCD graphics display, uses SPI bus         
    #define FEATURE_TOUCH_DISPLAY  // requires FEATURE_TFT_DISPLAY
    #define TOUCH_BUTTON_16
    #define TFT_320_480
    //#define FEATURE_STRAIGHT_KEY  // no pins left on this CPU, enable only when using the MCP23017 expander
    #define FEATURE_TFT_DISPLAY   // graphics, does not require touch
    //#define TOUCH_GT911_BUTTONS // use GT911 touch controller for buttons
    #define USE_RES_TOUCH // For XPT2046 resistive Touch Controller
    #define SET_CAL  // apply cal parameters set in keyer_pin_settings_esp32_dev.h file
    //#define CAL_TOUCH  // uncomment only for calibrating the display at startup, then comment out to run normal program.
    //#define USE_TOUCH_TASK  // run check_touch_buttons event handler in a task
    #define USE_BT_TASK // On pico this and touch interact and cause issues, panic for Ctrl-F-key combos (for now 2/2026)
    //#define USE_TASK
    //#define USE_GPS_TASK // !! ToDo : Not ready to use yet // run the Serial GPS in a separate task.
    #define USE_BLE  // uncomment to usee BLE, else use BT Classic keyboard
    //#define FEATURE_SINEWAVE_SIDETONE
    //#define FEATURE_SINEWAVE_SIDETONE_USING_TIMER_1
    //#define FEATURE_MCP23017_EXPANDER  // Add 16 external IO pins over I2C bus paddles and key on PA0-2
    #define FEATURE_COMPASS  // read magnetic compass and temperature on a GPS https://www.amazon.com/dp/B08NY9JSZ3
    #define FEATURE_GPS  // if enabled and not GPS, use DEFAULT_GRID = "" and supply memory 9 with a grid manually.
    #define GPS_BAUD_RATE 38400    // for the hardware serial port for GPS connection if used.
    #define GPS_SERIAL_INVERT 0   // invert the RX_pin signal if needed.  Common if connecting without buffers.
    //#define GPS_TEST  // uses nmea.h to provide simulated NMEA for CN87 or EM10 grids
    #define DEFAULT_GRID "CN87xs"  // substitute for GPS supplied grid square. This and GPS time will display on status row. Must be between 4 and 8 chars
    #define MAX_TX_PORTS 2  // nunmber of transmitter lines (TX) for TX Select touch button
    //#define OPTION_CW_DECODER_GOERTZEL_AUDIO_DETECTOR  // https://github.com/k3ng/k3ng_cw_keyer/wiki/385-Feature:-CW-Decoder 
    //#define FEATURE_CW_DECODER              // https://github.com/k3ng/k3ng_cw_keyer/wiki/385-Feature:-CW-Decoder  
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
#define OPTION_WINKEY_SEND_BREAKIN_STATUS_BYTE
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
//#define OPTION_CW_DECODER_GOERTZEL_AUDIO_DETECTOR  // https://github.com/k3ng/k3ng_cw_keyer/wiki/385-Feature:-CW-Decoder 
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
// #define OPTION_WORDSWORTH_POLISH

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


#endif // ESP_CONFIG_H