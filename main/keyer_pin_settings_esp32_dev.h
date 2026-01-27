/*

ESP32 dev board                                                                                  
See pin assignment and limitations: https://circuits4you.com/2018/12/31/esp32-devkit-esp32-wroom-gpio-pinout/
ADC2 is utylized by WiFi so if WiFi feature is used, ADC2 pins cannot be used for ADC inputs:
25, 26, 27, 14, 12, 13, 21, 22, 24, but can be used as digital i/o or PWM, interrupts etc.
*/
/* Pins - you must review these and configure ! */
#ifndef keyer_pin_settings_esdp32_dev_h
#define keyer_pin_settings_esdp32_dev_h

#ifdef FEATURE_MCP23017_EXPANDER
  // Add 100 o actual pin. 100 i sused o indicate an expansion pin and is subtracted in use
  #define MCP23017_PIN0     0  // paddle_left
  #define MCP23017_PIN1     1  // paddle right
  #define MCP23017_PIN2     2  // straight key
  // These next pins are future usage ideas, not implemeted yet.
  #define MCP23017_PIN3     3  // tx_key_line_1
  #define MCP23017_PIN4     4  // ptt_tx_1
  #define MCP23017_PIN5     5  // tx_key_line_2
  #define MCP23017_PIN6     6  // ptt_tx_2
  #define MCP23017_PIN7     7  // tx_key_line_3
  #define MCP23017_PIN8     8  // ptt_tx_3
  #define MCP23017_PIN9     9  // tx_key_line_4
  #define MCP23017_PIN10    10  // ptt_tx_4
  #define MCP23017_PIN11    11  // tx_key_line_5
  #define MCP23017_PIN12    12  // ptt_tx_5
  #define MCP23017_PIN13    13  // tx_key_line_6
  #define MCP23017_PIN14    14  // ptt_tx_6
  #define MCP23017_PIN15    15  // spare
#endif 

#define MAX_TX_PORTS 2  // nunmber of transmitter lines (TX) for TX Select touch button

// M5Stack Core2
//#define M5STACK_CORE2 // provides sound and LCD touchscreen in one module

#if defined (M5STACK_CORE2)
  #define bt_keyboard_LED 19  // indicates BT keyboard connection status
  #define paddle_left     27//32 Needs external 10k Pullup. 32 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
  #define paddle_right    33 //33 Needs external 10k Pullup. 33 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
  #define tx_key_line_1   26  // (high = key down/tx on)
  #define tx_key_line_2    0  
  #define sidetone_line    0 //23         // connect a speaker for sidetone
  #define audio_enable     0 // Only for 3.5" Hoysond Display
  //#define potentiometer   35 //A3 - VN pin // Speed potentiometer (0 to 3.3V) Use pot from 1k to 10k
  #define ptt_tx_1         0     //27   PTT ("push to talk") lines
  #define tx_inhibit_pin  13   //9
  #define tx_pause_pin    14  //10
  #ifdef FEATURE_STRAIGHT_KEY
    #define pin_straight_key 1 //TXD0
  #endif //FEATURE_STRAIGHT_KEY
  #define SD_SPI_CS_PIN    4
  #define SD_SPI_SCK_PIN  18
  #define SD_SPI_MISO_PIN 38
  #define SD_SPI_MOSI_PIN 23
  // 21,22 are i2c shared with touch, 39 is touch int.

  #elif defined (FEATURE_TFT7789_3_2inch_240x320_LCD)  // for DIY Malls 3.2" 320x240 TFT st7789
    #define bt_keyboard_LED 17 // indicates BT keyboard connection status - 17 on 3.2" DIY malls st7789 TFT, red BLUE    
    #ifdef FEATURE_MCP23017_EXPANDER
      #ifdef FEATURE_STRAIGHT_KEY
        #define pin_straight_key  MCP23017_PIN2 // expander pin PA2
      #endif //FEATURE_STRAIGHT_KEY
      #define MCP23X17_ADDR 0x27    
      #define MCP23017_I2C_PORT I2C_NUM_1
      #define MCP23017_INTA_GPIO 35 // input only, no pullup on this pin
      #define paddle_left     MCP23017_PIN0 // expander pin PA0
      #define paddle_right    MCP23017_PIN1 // expander pin PA1
      #define I2CDEV_SDA_PIN  21   // for extrnal i2c modules
      #define I2CDEV_SCL_PIN  22
    #else
      #define paddle_left     21 //32 Needs external 10k Pullup. 32 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
      #define paddle_right    22 //33 Needs external 10k Pullup. 33 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
      #ifdef FEATURE_STRAIGHT_KEY
        #define pin_straight_key  25 //TXD0
      #endif //FEATURE_STRAIGHT_KEY
      #define GPS_RX_PIN      35 // will conflict with NCP23017 if used
    #endif
    #define TOUCH_SDA  33  // intenral bus for i2c touch controller GT911 pins
    #define TOUCH_SCL  32
    #define TOUCH_INT  21  // not actually used because R25 is not installed on DIYMalls 3.2" display and 21 is for i2c
    #define TOUCH_RST  25
    #define TOUCH_WIDTH  320  
    #define TOUCH_HEIGHT 240
    #define potentiometer    0 // 35 Speed potentiometer (0 to 3.3V) Use pot from 1k to 10k
    #define tx_key_line_1   16 // (high = key down/tx on) (16 on 3.2" DIY malls st7789 TFT, green LED)
    #define tx_key_line_2   16 
    #define sidetone_line   26 // connect a speaker for sidetone
    #define audio_enable     0 // Only for 3.5" Hoysond Display
    #define potentiometer    0 //A3 - VN pin // Speed potentiometer (0 to 3.3V) Use pot from 1k to 10k
    #define ptt_tx_1         4 // PTT ("push to talk") lines   (4 on 3.2" DIY malls st7789 TFT, red LED)
    #define tx_inhibit_pin   0 //13 ((2, 27, 12-15 used for 3.2" DIY Malls st7789 TFT)
    #define tx_pause_pin     0 //14 

    #elif defined (FEATURE_IDEASPARK_LCD)
    #define bt_keyboard_LED 13  // indicates BT keyboard connection status pin ? on IdeaSpark 1.9" onboard Blue LED
    #define paddle_left     25 //32 Needs external 10k Pullup. 32 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
    #define paddle_right    26 //33 Needs external 10k Pullup. 33 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
    #define tx_key_line_1   16 // 16 aka RX2   (high = key down/tx on)
    #define tx_key_line_2    0 
    #define sidetone_line   17 // aka TX2   connect a passive buzzer for sidetone
    #define audio_enable     0 // Only for 3.5" Hoysond Display
    #define GPS_RX_PIN      35 // will conflict pot if used
    #define potentiometer    0 //35 Speed potentiometer (0 to 3.3V) Use pot from 1k to 10k
    #define ptt_tx_1         5 // PTT ("push to talk") lines 
    #define tx_inhibit_pin   0  //13 
    #define tx_pause_pin     0  //14
    #ifdef FEATURE_STRAIGHT_KEY
      #define pin_straight_key 27
    #endif //FEATURE_STRAIGHT_KEY
    
    // following linbes are used for internal functions
    //#define LCD_H_RES      320
    //#define LCD_V_RES      240
    //#define LCD_HOST VSPI_HOST
    //#define LCD_BUF_LINES   60
    //#define LV_TICK_PERIOD_MS 1
    //#define SD_SPI_CS_PIN   15
    //#define SD_SPI_SCK_PIN  18
    //#define SD_SPI_MISO_PIN -1
    //#define SD_SPI_MOSI_PIN 23
    //#define SD_SPI_RST_PIN   4
    //#define SD_SPI_BLK_PIN  32
    //#define SD_SPI_DC_PIN    2
  // 36 ADC input free - input only

#elif defined (FEATURE_TFT_HOSYOND_320x480_LCD)
    #define bt_keyboard_LED 17  // indicates BT keyboard connection status - GPIO17 is Blue LED
    #ifdef FEATURE_MCP23017_EXPANDER
      #define MCP23X17_ADDR 0x27
      #define MCP23017_I2C_PORT I2C_NUM_0
      #define MCP23017_INTA_GPIO 35 // input only, no pullup on this pin
      #define paddle_left     MCP23017_PIN0 // expander pin PA0
      #define paddle_right    MCP23017_PIN1 // expander pin PA1
      #define I2CDEV_SDA_PIN  32   // override the sdkconfig pair of 21, 22
      #define I2CDEV_SCL_PIN  25
      #ifdef FEATURE_STRAIGHT_KEY
        #define pin_straight_key  MCP23017_PIN2 // expander pin PA2
      #endif //FEATURE_STRAIGHT_KEY
    #else
      #define paddle_left     32 //32 Needs external 10k Pullup. 32 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
      #define paddle_right    25 //33 Needs external 10k Pullup. 33 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
      #ifdef FEATURE_STRAIGHT_KEY
        #define pin_straight_key  39
      #endif //FEATURE_STRAIGHT_KEY
      #define GPS_RX_PIN      35 // will conflict with NCP23017 if used      
    #endif
    #define TOUCH_X         300     // x  300 default
    #define TOUCH_X1        3598    // x1 3600 default
    #define TOUCH_Y         345     // y  300 default
    #define TOUCH_Y1        3549    // y1 3600 default
    #define TOUCH_BITS      (0x01) // param bits are bool: rotate=bit0, invertx=bit1, inverty=bit2
    #define potentiometer    0 // 35 Speed potentiometer (0 to 3.3V) Use pot from 1k to 10k
    #define tx_key_line_1   18 // (high = key down/tx on)
    #define tx_key_line_2   16 
    #define sidetone_line   26 //26 connect a passive buzzer for sidetone
    #define audio_enable     4  // audio amp enable = LOW for this display (only?)
    #define ptt_tx_1        22 // 23 for PTT ("push to talk") lines, GPIO22 is RED LED
    #define tx_inhibit_pin  0 //19 
    #define tx_pause_pin    0 //23
    #ifdef FEATURE_STRAIGHT_KEY
      #define pin_straight_key 39 //TXD0 - GREEN LED
    #endif //FEATURE_STRAIGHT_KEY
    // 4 connectors for IO. | 23, 19, 18, 21 || 35, 39 || 32, 25, 3.3V, GND |
    // 25/32/3.3/GND and 39 used for I2C for port expander if used.  
    // 25/32 and 39 used for paddles and key if no port expander
    // 35 is speed pot if used fo both
    // 18/23  is Tx #1 Tx_Key_line and PTT out
    // 22 is RED LED, 17 is BLUE LED, and 16 is GREEN LED
    // --->   that leaves onboard pins 19 21 free  in both cases  <-----

    #elif defined (FEATURE_TFT_320x480_CAP_LCD)    // for ESP32-3248S05C 3.5" 320x480 TFT ST7796 and GT911
    #define bt_keyboard_LED 17 // indicates BT keyboard connection status - 17 on 3.2" DIY malls st7789 TFT, red BLUE    
    #ifdef FEATURE_MCP23017_EXPANDER
      #ifdef FEATURE_STRAIGHT_KEY
        #define pin_straight_key  MCP23017_PIN2 // expander pin PA2
      #endif //FEATURE_STRAIGHT_KEY
      #define MCP23X17_ADDR 0x27    
      #define MCP23017_I2C_PORT I2C_NUM_1
      #define MCP23017_INTA_GPIO 35 // input only, no pullup on this pin
      #define paddle_left     MCP23017_PIN0 // expander pin PA0
      #define paddle_right    MCP23017_PIN1 // expander pin PA1
      #define I2CDEV_SDA_PIN  21   // for extrnal i2c modules
      #define I2CDEV_SCL_PIN  22
    #else
      #define paddle_left     18 //32 Needs external 10k Pullup. 32 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
      #define paddle_right    19 //33 Needs external 10k Pullup. 33 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
      #ifdef FEATURE_STRAIGHT_KEY
        #define pin_straight_key  25 //TXD0
      #endif //FEATURE_STRAIGHT_KEY
      #define GPS_RX_PIN      35 // will conflict with NCP23017 if used     
    #endif
    #define TOUCH_SDA  33  // intenral bus for i2c touch controller GT911 pins
    #define TOUCH_SCL  32
    #define TOUCH_INT  21  // not actually used because R25 is not installed on DIYMalls 3.2" display and 21 is for i2c
    #define TOUCH_RST  25
    #define TOUCH_WIDTH  480  
    #define TOUCH_HEIGHT 320
    #define tx_key_line_1   22 // (high = key down/tx on) (16 on 3.2" DIY malls st7789 TFT, green LED)
    #define tx_key_line_2   16 
    #define sidetone_line   26 // connect a speaker for sidetone
    #define audio_enable     0 // Only for 3.5" Hoysond Display
    #define potentiometer    0 //A3 - VN pin // Speed potentiometer (0 to 3.3V) Use pot from 1k to 10k
    #define ptt_tx_1         4 // PTT ("push to talk") lines   (4 on 3.2" DIY malls st7789 TFT, red LED)
    #define tx_inhibit_pin   0 //13 ((2, 27, 12-15 used for 3.2" DIY Malls st7789 TFT)
    #define tx_pause_pin     0 //14 

#else // ESP-WROOM-32, 27, 5 33
  #define bt_keyboard_LED   2 //2  // indicates BT keyboard connection status
  #define paddle_left       25 //32 Needs external 10k Pullup. 32 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
  #define paddle_right      26 //33 Needs external 10k Pullup. 33 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
  #define tx_key_line_1     17  // (high = key down/tx on)
  #define sidetone_line     14 //23         // connect a speaker for sidetone
  #define audio_enable      0 // Only for 3.5" Hoysond Display
  #define potentiometer     36 //A3 - VN pin // Speed potentiometer (0 to 3.3V) Use pot from 1k to 10k
  #define ptt_tx_1          16             // PTT ("push to talk") lines
  #define tx_key_line_2     0 
  #define tx_inhibit_pin    0
  #define tx_pause_pin      0
  #ifdef FEATURE_STRAIGHT_KEY
    #define pin_straight_key 27 //TXD0
  #endif //FEATURE_STRAIGHT_KEY
  #define GPS_RX_PIN        35 // will conflict with NCP23017 if used     
#endif

//  common pins
#define tx_key_line_3 0
#define tx_key_line_4 0
#define tx_key_line_5 0
#define tx_key_line_6 0

#define ptt_tx_2 0             //   Can be used for keying fox transmitter, T/R switch, or keying slow boatanchors
#define ptt_tx_3 0              //   These are optional - set to 0 if unused
#define ptt_tx_4 0
#define ptt_tx_5 0
#define ptt_tx_6 0
#define tx_key_dit 0            // if defined, goes active for dit (any transmitter) - customized with tx_key_dit_and_dah_pins_active_state and tx_key_dit_and_dah_pins_inactive_state
#define tx_key_dah 0            // if defined, goes active for dah (any transmitter) - customized with tx_key_dit_and_dah_pins_active_state and tx_key_dit_and_dah_pins_inactive_state

#define potentiometer_enable_pin 0  // if defined, the potentiometer will be enabled only when this pin is held low; set to 0 to ignore this pin

#ifdef FEATURE_BUTTONS
  #define analog_buttons_pin 0 //A0 VP pin on the board Not all pins of ESP32 are available for ADC. See https://circuits4you.com/2018/12/31/esp32-devkit-esp32-wroom-gpio-pinout/
  #define command_mode_active_led 27 //for ESP32_DEV board On Board LED
#endif //FEATURE_BUTTONS

/*
FEATURE_SIDETONE_SWITCH
  Enabling this feature and an external toggle switch  adds switch control for playing cw sidetone.
  ST Switch status is displayed in the status command.  This feature will override the software control of the sidetone (\o).
  Arduino pin is assigned by SIDETONE_SWITCH 
*/

#ifdef FEATURE_SIDETONE_SWITCH
  #define SIDETONE_SWITCH 0
#endif //FEATURE_SIDETONE_SWITCH


//lcd pins
#if defined (FEATURE_LCD_4BIT) || defined(FEATURE_LCD_8BIT) //corrected by SP5IOU 20210802
  #define lcd_rs 4 //32
  #define lcd_enable 16 //RX2 pin
  #define lcd_d4 17 //TX2 pin
  #define lcd_d5 5
  #define lcd_d6 18
  #define lcd_d7 19
#endif //FEATURE_LCD_4BIT

#if defined(FEATURE_LCD_8BIT) // addition four data lines for 8 bit LCD control
  #define lcd_d0 13 // 13 Note this pin can be used for Inline debugger
  #define lcd_d1 12 // 12 Note this pin can be used for Inline debugger
  #define lcd_d2 14 // 14 Note this pin can be used for Inline debugger
  #define lcd_d3 15 // 15 Note this pin can be used for Inline debugger
#endif //FEATURE_LCD_4BIT || defined(FEATURE_LCD_8BIT)

#ifdef FEATURE_LCD1602_N07DH
  #define lcd_rs 4 //32
  #define lcd_enable 16 //RX2 pin
  #define lcd_d4 17 //TX2 pin
  #define lcd_d5 5
  #define lcd_d6 18
  #define lcd_d7 19
#endif //FEATURE_LCD1602_N07DH

//ps2 keyboard pins
#ifdef FEATURE_PS2_KEYBOARD
  #define ps2_keyboard_data 13 
  #define ps2_keyboard_clock 12    // this must be on an interrupt capable pin!
#endif //FEATURE_PS2_KEYBOARD

// rotary encoder pins and options - rotary encoder code from Jim Balls M0CKE
#ifdef FEATURE_ROTARY_ENCODER
  #define OPTION_ENCODER_HALF_STEP_MODE     // Half-step mode?
  #define rotary_pin1  34 //34 Needs external 10k Pullup   // CW Encoder Pin  Not all ESP32 pins supports PULLUP SP5IOU
  #define rotary_pin2  35 //35 Needs external 10k Pullup   // CCW Encoder Pin  Not all ESP32 pins supports PULLUP SP5IOU
  #define OPTION_ENCODER_ENABLE_PULLUPS     // define to enable weak pullups.
#endif //FEATURE_ROTARY_ENCODER

#ifdef FEATURE_LED_RING
  #define led_ring_sdi    0    //Data
  #define led_ring_clk    0    //Clock
  #define led_ring_le     0    //Latch
#endif //FEATURE_LED_RING

#define correct_answer_led 0
#define wrong_answer_led 0

#ifdef FEATURE_PTT_INTERLOCK
  #define ptt_interlock 0  // this pin disables PTT and TX KEY
#endif //FEATURE_PTT_INTERLOCK

// FEATURE_CW_DECODER & OPTION_CW_DECODER_GOERTZEL_AUDIO_DETECTOR
// See https://github.com/k3ng/k3ng_cw_keyer/wiki/385-Feature:-CW-Decoder for details
//#define cw_decoder_pin 13             // This is for use with external decoding hardware
//#define cw_decoder_audio_input_pin 39 // 0 // This is for audio detection decoding using OPTION_CW_DECODER_GOERTZEL_AUDIO_DETECTOR; this must be an analog pin!
//#define cw_decoder_indicator 0       // Output - goes HIGH when cw tone is detected by OPTION_CW_DECODER_GOERTZEL_AUDIO_DETECTOR


#if defined(FEATURE_COMPETITION_COMPRESSION_DETECTION)
  #define compression_detection_pin 0
#endif //FEATURE_COMPETITION_COMPRESSION_DETECTION

#if defined(FEATURE_SLEEP)
  #define keyer_awake paddle_right //Press paddle right to awake keyer (Hardware esp32_dev)
#endif

#if defined(FEATURE_LCD_BACKLIGHT_AUTO_DIM)
  #define keyer_power_led 15   // must be a PWM-capable pin
#endif

#if defined(FEATURE_CAPACITIVE_PADDLE_PINS)
  #define capactive_paddle_pin_inhibit_pin 0     // if this pin is defined and is set high, the capacitive paddle pins will switch to normal (non-capacitive) sensing mode
#endif

#if defined(FEATURE_TOUCH_PADDLE_PINS)

#endif
#ifdef FEATURE_4x4_KEYPAD
  #define Row3 0
  #define Row2 0
  #define Row1 0
  #define Row0 0
  #define Col3 0
  #define Col2 0
  #define Col1 0
  #define Col0 0
#endif

#ifdef FEATURE_3x4_KEYPAD
  #define Row3 0
  #define Row2 0
  #define Row1 0
  #define Row0 0
  #define Col2 0
  #define Col1 0
  #define Col0 0
#endif

#ifdef FEATURE_SEQUENCER
  #define sequencer_1_pin 0
  #define sequencer_2_pin 0
  #define sequencer_3_pin 0
  #define sequencer_4_pin 0
  #define sequencer_5_pin 0
#endif //FEATURE_SEQUENCER

#define ptt_input_pin 0

#define pin_sending_mode_automatic 0  // goes HIGH when keyer is sending code automatically
#define pin_sending_mode_manual 0     // goes HIGH when keyer is sending code manually (i.e. the paddle or straight key)

#else

  #error "Multiple pin_settings.h files included somehow..."

#endif //keyer_pin_settings_esdp32_dev_h
