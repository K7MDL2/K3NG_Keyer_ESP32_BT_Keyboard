/*

ESP32 dev board                                                                                  
See pin assignment and limitations: https://circuits4you.com/2018/12/31/esp32-devkit-esp32-wroom-gpio-pinout/
ADC2 is utylized by WiFi so if WiFi feature is used, ADC2 pins cannot be used for ADC inputs:
25, 26, 27, 14, 12, 13, 21, 22, 24, but can be used as digital i/o or PWM, interrupts etc.
*/
/* Pins - you must review these and configure ! */
#ifndef keyer_pin_settings_pico_h
#define keyer_pin_settings_pico_h

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

// M5Stack Core2
//#define M5STACK_CORE2 // provides sound and LCD touchscreen in one module

#if defined (FEATURE_TFT_PICO_320x480_RES_LCD)
    #define bt_keyboard_LED    LED_BUILTIN //64 // indicates BT keyboard connection status - GPIO17 is Blue LED

    #if defined (FEATURE_MCP23017_EXPANDER) || defined (FEATURE_COMPASS)
      #define I2CDEV_SDA_PIN  20   // override the sdkconfig pair of 21, 22
      #define I2CDEV_SCL_PIN  21
    #endif

    #ifdef FEATURE_MCP23017_EXPANDER
      #define MCP23X17_ADDR 0x27      
      #define MCP23017_INTA_GPIO 22 // input only, no pullup on this pin
      #define paddle_left     MCP23017_PIN0 // expander pin PA0
      #define paddle_right    MCP23017_PIN1 // expander pin PA1
      #ifdef FEATURE_STRAIGHT_KEY
        #define pin_straight_key  MCP23017_PIN2 // expander pin PA2
      #endif //FEATURE_STRAIGHT_KEY
    #else
      #define paddle_left      2 // Must have pullup internal or external
      #define paddle_right     3 // Must have pullup internal or external.
      #ifdef FEATURE_STRAIGHT_KEY
        #define pin_straight_key  22
      #endif //FEATURE_STRAIGHT_KEY          
    #endif
    #define GPS_RX_PIN      5   // UART1 TX = 4   UART1 RX = 5
    #define GPS_TX_PIN      4
    #define TOUCH_X         4400     // x  300 default
    #define TOUCH_X1        3550    // x1 3600 default
    #define TOUCH_Y         4300     // y  300 default
    #define TOUCH_Y1        3550    // y1 3600 default
    #define TOUCH_BITS      (0x05) // param bits are bool: rotate=bit0, invertx=bit1, inverty=bit2
    #define potentiometer   26 // ADC0 = Speed potentiometer (0 to 3.3V) Use pot from 1k to 10k
    #define tx_key_line_1    6 // (high = key down/tx on)
    #define tx_key_line_2   14 
    #define sidetone_line   42 // 18  //26 connect a passive buzzer for sidetone   42 is RP2350B
    #define audio_enable     0  // audio amp enable = LOW for this display (only?)
    #define ptt_tx_1        19  // 22 for PTT ("push to talk") lines, GPIO22 is RED LED
    #define ptt_tx_2         7  //   Can be used for keying fox transmitter, T/R switch, or keying slow boatanchors
    #define tx_inhibit_pin   0 
    #define tx_pause_pin     0 
    #ifdef FEATURE_STRAIGHT_KEY
      #define pin_straight_key 27 //TXD0 - GREEN LED
    #endif //FEATURE_STRAIGHT_KEY
    // free pins are 7, 14, 16 (UART1 TX).
    // --->   that leaves onboard pins 19 21 free  in both cases  <-----
    // FEATURE_CW_DECODER & OPTION_CW_DECODER_GOERTZEL_AUDIO_DETECTOR
    // See https://github.com/k3ng/k3ng_cw_keyer/wiki/385-Feature:-CW-Decoder for details
    //#define cw_decoder_pin 13             // This is for use with external decoding hardware
    //#define cw_decoder_audio_input_pin 39 // 0 // This is for audio detection decoding using OPTION_CW_DECODER_GOERTZEL_AUDIO_DETECTOR; this must be an analog pin!
    //#define cw_decoder_indicator 16       // Output - goes HIGH when cw tone is detected by OPTION_CW_DECODER_GOERTZEL_AUDIO_DETECTOR
    
    // following linbes are used for internal functions
    //#define LCD_H_RES      480
    //#define LCD_V_RES      320
    //#define SPI_PORT      spi1   // Waveshare 3.5" IL9488 uses spi1 bus for lcd

    // 0,1 are UART0, likely parallel with USB.
    // 2,3,4,6,7, 14 are free
    //#define TFT_SPI_CS_PIN    9
    //#define TFT_SPI_SCK_PIN  10
    //#define TFT_SPI_MISO_PIN 12
    //#define TFT_SPI_MOSI_PIN 11
    //#define TFT_SPI_RST_PIN  15
    //#define TFT_SPI_BLK_PIN  13
    //#define TFT_SPI_DC_PIN    8
    
    //#define TP_CS_PIN        16
    //#define TP_IRQ_PIN       17
    
    //#define SDIO_CLK_PIN      5
    //#define SDIO_CS_PIN      18
    //#define SDIO_D0_PIN      19
    //#define SDIO_D1_PIN      20  // using for i2c
    //#define SDIO_D2_PIN      21  // using for i2c
    //#define SDIO_CMD_D3_PIN  22
    // 26, 27, 28 are ADC capable.
    
#else // ESP-WROOM-32, 27, 5 33

  #define I2CDEV_SDA_PIN  21   // override the sdkconfig pair of 21, 22
  #define I2CDEV_SCL_PIN  22
  
  #define bt_keyboard_LED   2 //2  // indicates BT keyboard connection status
  #define paddle_left       25 //32 Needs external 10k Pullup. 32 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
  #define paddle_right      26 //33 Needs external 10k Pullup. 33 can be used as touch paddle on ESP32 platform //SP5IOU 20220201
  #define tx_key_line_1     17  // (high = key down/tx on)
  #define sidetone_line     14 //23         // connect a speaker for sidetone
  #define audio_enable      0 // Only for 3.5" Hoysond Display
  #define potentiometer     36 //A3 - VN pin // Speed potentiometer (0 to 3.3V) Use pot from 1k to 10k
  #define ptt_tx_1          16             // PTT ("push to talk") lines
  #define ptt_tx_2          0 //   Can be used for keying fox transmitter, T/R switch, or keying slow boatanchors
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
