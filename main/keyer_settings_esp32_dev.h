/*

*/

// Initial and hardcoded settings
#define initial_speed_wpm 13             // "factory default" keyer speed setting
#define initial_command_mode_speed_wpm 20 // "factory default" command mode speed setting 
#define initial_sidetone_freq 700        // "factory default" sidetone frequency setting
#define sidetone_hz_limit_low 299
#define sidetone_hz_limit_high 2001
#define hz_high_beep 1500                // frequency in hertz of high beep
#define hz_low_beep 400                  // frequency in hertz of low beep
#define initial_dah_to_dit_ratio 300     // 300 = 3 / normal 3:1 ratio
#define initial_ptt_lead_time_tx1 30         // PTT lead time in mS
#define initial_ptt_tail_time_tx1 30 //10         // PTT tail time in mS //SP5IOU - better to change it by CLI
#define initial_ptt_lead_time_tx2 0         // PTT lead time in mS
#define initial_ptt_tail_time_tx2 0 //10         // PTT tail time in mS
#define initial_ptt_lead_time_tx3 0         // PTT lead time in mS
#define initial_ptt_tail_time_tx3 0 //10         // PTT tail time in mS
#define initial_ptt_lead_time_tx4 0         // PTT lead time in mS
#define initial_ptt_tail_time_tx4 0 //10         // PTT tail time in mS
#define initial_ptt_lead_time_tx5 0         // PTT lead time in mS
#define initial_ptt_tail_time_tx5 0 //10         // PTT tail time in mS
#define initial_ptt_lead_time_tx6 0         // PTT lead time in mS
#define initial_ptt_tail_time_tx6 0 //10         // PTT tail time in mS
#define initial_qrss_dit_length 1        // QRSS dit length in seconds
#define initial_pot_wpm_low_value 6 //13     // Potentiometer WPM fully CCW //SP5IOU to narrow range
#define initial_pot_wpm_high_value 30 //35    // Potentiometer WPM fully CW
#define wpm_limit_low 5
#define wpm_limit_high 50
#define potentiometer_change_threshold 0.9 // don't change the keyer speed until pot wpm has changed more than this
#define send_buffer_size 150
#define default_length_letterspace 3
#define default_length_wordspace 7
#define default_keying_compensation 0    // number of milliseconds to extend all dits and dahs - for QSK on boatanchors
#define default_first_extension_time 0   // number of milliseconds to extend first sent dit or dah
//#define default_pot_full_scale_reading 1023 //For AVR boards
#define default_pot_full_scale_reading 4095 //SP5IOU 20180329 FOR stm32 and esp32 Boards.
#define default_weighting 50             // 50 = weighting factor of 1 (normal)
#define default_ptt_hang_time_wordspace_units 1.0
#define winkey_c0_wait_time 1            // the number of milliseconds to wait to send 0xc0 byte after send buffer has been sent
#define winkey_command_timeout_ms 5000
#define winkey_discard_bytes_startup 3   // this is used if OPTION_WINKEY_DISCARD_BYTES_AT_STARTUP is enabled above
#define winkey_xoff_threshold 20         // the number of chars in the buffer when we begin sending XOFFs
#define winkey_xon_threshold 10          // the number of chars in the buffer below which we deactivate XOFF
#define default_memory_repeat_time 3000  // time in milliseconds

#ifdef TFT_320_480   // for 480px wide display
  #define LCD_COLUMNS 20  // sets the char per line in cw text scroll box
  #define LCD_ROWS 3      // number of rows in teh CW Scroill Box
#elif defined(FEATURE_TFT_DISPLAY)  // all 320px wideTFT displays
  #define LCD_COLUMNS 18  // sets the char per line in cw text scroll box
  #define LCD_ROWS 5      // number of rows in the CW Scroill Box
#else
  #define LCD_COLUMNS 20
  #define LCD_ROWS 4
#endif

#if defined(FEATURE_PS2_KEYBOARD) || defined(FEATURE_TOUCH_DISPLAY)
  #define PS2_KEYBOARD_NORMAL 0
#endif //FEATURE_PS2_KEYBOARD

#if defined(FEATURE_BT_KEYBOARD) || defined(FEATURE_TOUCH_DISPLAY)
  #define BT_KEYBOARD_NORMAL 0
#endif //FEATURE_BT_KEYBOARD
 
#define lcd_i2c_address_mathertel_PCF8574 0x27             // I2C address of display for FEATURE_LCD_MATHERTEL_PCF8574
#define lcd_i2c_address_fdebrander_lcd 0x27                // I2C address of display for FEATURE_LCD_I2C_FDEBRABANDER
#define lcd_i2c_address_ydv1_lcd 0x27                      // I2C address of display for FEATURE_LCD_YDv1
//#define lcd_i2c_address_ydv1_lcd 0x38                    // I2C address of display for FEATURE_LCD_YDv1
#define lcd_i2c_address_sainsmart_lcd 0x27                // I2C address of display for FEATURE_LCD_SAINSMART_I2C 
#define hell_pixel_microseconds 4025
#define program_memory_limit_consec_spaces 1
#define serial_leading_zeros 1            // set to 1 to activate leading zeros in serial numbers (i.e. #1 = 001)
#define serial_cut_numbers 0              // set to 1 to activate cut numbers in serial numbers (i.e. #10 = 1T, #19 = 1N)
#define go_to_sleep_inactivity_time 10    // minutes - FEATURE_SLEEP
#define dim_backlight_inactive_time 1 //5     // minutes - FEATURE_LCD_BACKLIGHT_AUTO_DIM
#define default_cmos_super_keyer_iambic_b_timing_percent 33 // use with FEATURE_CMOS_SUPER_KEYER_IAMBIC_B_TIMING; should be between 0 to 99 % (0% = true iambic b;100% = iambic a behavior)
#define default_cw_echo_timing_factor 1.75 // "factory default" setting
#define default_autospace_timing_factor 2.0 // "factory default" setting
#define winkey_paddle_echo_buffer_decode_timing_factor 0.25
#define potentiometer_always_on 1 //0 //On ESP32 0 didn't work
#define ptt_interlock_check_every_ms 100
#define ptt_interlock_active_state HIGH
#define unknown_cw_character '*'
#define cli_paddle_echo_on_at_boot 1
#define cli_straight_key_echo_on_at_boot 1
#define tx_key_dit_and_dah_pins_active_state LOW
#define tx_key_dit_and_dah_pins_inactive_state HIGH
#define potentiometer_check_interval_ms 150
#define potentiometer_reading_threshold 1 
#define default_paddle_interruption_quiet_time_element_lengths 0
#define default_wordsworth_wordspace 6
#define default_wordsworth_repetition 1
#define serial_program_memory_buffer_size 500
#define eeprom_write_time_ms 30000

#ifdef FEATURE_BUTTONS
  #define analog_buttons_number_of_buttons 9 // For Keypad only command button is neaded 6 //4  // includes the command button (command button + 3 memory buttons = 4) //sp5iou 20180319
  #define analog_buttons_r1 11.5
  #define analog_buttons_r2 1
#endif


#if defined(FEATURE_BUTTONS) &&  !defined(FEATURE_PS2_KEYBOARD) && !defined(FEATURE_USB_KEYBOARD) && !defined(FEATURE_COMMAND_LINE_INTERFACE) && !defined(FEATURE_WINKEY_EMULATION)
  #define number_of_memories byte(analog_buttons_number_of_buttons-1)
#else
  #define number_of_memories byte(12) //sp5iou 20220124 With many memories, be carefull to not put to much content. It caould disable memory programming and eeprom formatting is then necessary
#endif

#ifdef FEATURE_CAPACITIVE_PADDLE_PINS
  #define capacitance_threshold 2
#endif //FEATURE_CAPACITIVE_PADDLE_PINS

#ifdef FEATURE_TOUCH_PADDLE_PINS //SP5IOU 20220131  Works on ESP32_DEV platform
#define touch_threshold 20 //Set it according to touch paddles construction. Smaller value - less sensitive.
#endif

#ifdef FEATURE_LED_RING
  #define led_ring_low_limit 10
  #define led_ring_high_limit 50
#endif //FEATURE_LED_RING

#ifdef FEATURE_QLF
  #define qlf_dit_max 125
  #define qlf_dit_min 75
  #define qlf_dah_max 200
  #define qlf_dah_min 100
  #define qlf_on_by_default 0
#endif //FEATURE_QLF

#ifdef FEATURE_WINKEY_EMULATION
  #ifdef OPTION_WINKEY_UCXLOG_9600_BAUD
    #define WINKEY_DEFAULT_BAUD 9600
  #else
    #define WINKEY_DEFAULT_BAUD 1200
  #endif //OPTION_WINKEY_UCXLOG_9600_BAUD
// alter these below to map alternate sidetones for Winkey interface protocol emulation
#ifdef OPTION_WINKEY_2_SUPPORT
	#define WINKEY_SIDETONE_1 3759
	#define WINKEY_SIDETONE_2 1879
	#define WINKEY_SIDETONE_3 1252
	#define WINKEY_SIDETONE_4 940
	#define WINKEY_SIDETONE_5 752
	#define WINKEY_SIDETONE_6 625
	#define WINKEY_SIDETONE_7 535
	#define WINKEY_SIDETONE_8 469
	#define WINKEY_SIDETONE_9 417
	#define WINKEY_SIDETONE_10 375
#else //OPTION_WINKEY_2_SUPPORT
	#define WINKEY_SIDETONE_1 4000
	#define WINKEY_SIDETONE_2 2000
	#define WINKEY_SIDETONE_3 1333
	#define WINKEY_SIDETONE_4 1000
	#define WINKEY_SIDETONE_5 800
	#define WINKEY_SIDETONE_6 666
	#define WINKEY_SIDETONE_7 571
	#define WINKEY_SIDETONE_8 500
	#define WINKEY_SIDETONE_9 444
	#define WINKEY_SIDETONE_10 400
#endif //OPTION_WINKEY_2_SUPPORT

#define WINKEY_1_REPORT_VERSION_NUMBER 10
#define WINKEY_2_REPORT_VERSION_NUMBER 23

// alter these to map to alternate hang time wordspace units
#define WINKEY_HANG_TIME_1_0 1.0
#define WINKEY_HANG_TIME_1_33 1.33
#define WINKEY_HANG_TIME_1_66 1.66
#define WINKEY_HANG_TIME_2_0 2.0

#define WINKEY_RETURN_THIS_FOR_ADMIN_GET_CAL_WK1 0x0a
#define WINKEY_RETURN_THIS_FOR_ADMIN_GET_CAL_WK2 0x18
#define WINKEY_RETURN_THIS_FOR_ADMIN_PADDLE_A2D 0xEE
#define WINKEY_RETURN_THIS_FOR_ADMIN_SPEED_A2D 0x00

#endif //FEATURE_WINKEY_EMULATION

#define PRIMARY_SERIAL_PORT &Serial
#define PRIMARY_SERIAL_PORT_BAUD 115200     // This applies only when the port is in Command Line Interface mode.  In Winkey mode it will default to 1200.

#define GPS_SERIAL_PORT &Serial2
#define GPS_SERIAL_PORT_BAUD 115200     // This applies only when the port is in Command Line Interface mode.  In Winkey mode it will default to 1200.

#ifdef FEATURE_COMMAND_LINE_INTERFACE_ON_SECONDARY_PORT
  #define SECONDARY_SERIAL_PORT &Serial1
  #define SECONDARY_SERIAL_PORT_BAUD 115200
#endif

#define CW_DECODER_SCREEN_COLUMNS 120        // word wrap at this many columns
#define CW_DECODER_SPACE_PRINT_THRESH 4.5   // print space if no tone for this many element lengths
#define CW_DECODER_SPACE_DECODE_THRESH 2.0  // invoke character decode if no tone for this many element lengths
#define CW_DECODER_NOISE_FILTER 20          // ignore elements shorter than this (mS)

#define STRAIGHT_KEY_ACTIVE_STATE LOW

#ifdef FEATURE_DYNAMIC_DAH_TO_DIT_RATIO
  #define DYNAMIC_DAH_TO_DIT_RATIO_LOWER_LIMIT_WPM 30
  #define DYNAMIC_DAH_TO_DIT_RATIO_LOWER_LIMIT_RATIO 300 // 300 = 3:1 ratio
  #define DYNAMIC_DAH_TO_DIT_RATIO_UPPER_LIMIT_WPM 70
  #define DYNAMIC_DAH_TO_DIT_RATIO_UPPER_LIMIT_RATIO 240 // 240 = 2.4:1 ratio
#endif //FEATURE_DYNAMIC_DAH_TO_DIT_RATIO

#if defined(FEATURE_COMPETITION_COMPRESSION_DETECTION)
  #define COMPETITION_COMPRESSION_DETECTION_ARRAY_SIZE 16
  #define COMPETITION_COMPRESSION_DETECTION_TIME_INTERCHAR_LOWER_LIMIT 1
  #define COMPETITION_COMPRESSION_DETECTION_TIME_INTERCHAR_UPPER_LIMIT 6.0
  #define COMPETITION_COMPRESSION_DETECTION_AVERAGE_ALARM_THRESHOLD 3.0
#endif //FEATURE_COMPETITION_COMPRESSION_DETECTION

#if defined(FEATURE_SLEEP)
  #define KEYER_AWAKE_PIN_AWAKE_STATE HIGH
  #define KEYER_AWAKE_PIN_ASLEEP_STATE LOW
#endif 

#if defined(FEATURE_LCD_BACKLIGHT_AUTO_DIM)
  #define keyer_power_led_awake_duty 30 //255   // PWM duty cycle. 0 is 0%, 255 is 100%
  #define keyer_power_led_asleep_duty 2 //25   // 25 is quite dim. Use 0 for off
#endif
#if defined FEATURE_WIFI
#define WIFI_SSID "YourSSID" //SSID of Your access point
#define WIFI_PASSWORD "Yourpassword" //Password of Your access point
#define WIFI_ATTEMPTS 30 //How many tries to connect.
#endif
#if defined(FEATURE_ETHERNET)
  // #define FEATURE_ETHERNET_IP {192,168,1,178}                      // default IP address ("192.168.1.178")
  // #define FEATURE_ETHERNET_MAC {0xDE,0xAD,0xBE,0xEF,0xFE,0xED}
  #define FEATURE_ETHERNET_IP {192,168,1,179}                      // default IP address ("192.168.1.179")
  #define FEATURE_ETHERNET_MAC {0xDE,0xAD,0xBE,0xEF,0xFE,0xEE}
  #define FEATURE_ETHERNET_DNS {8,8,8,8} 

  #define FEATURE_ETHERNET_GATEWAY {192,168,1,1}                   // default gateway
  #define FEATURE_ETHERNET_SUBNET_MASK {255,255,255,0}                  // default subnet mask
  #define FEATURE_ETHERNET_WEB_LISTENER_PORT 80
  #define FEATURE_UDP_SEND_BUFFER_SIZE 128
  #define FEATURE_UDP_RECEIVE_BUFFER_SIZE 128
#endif //FEATURE_ETHERNET

#define WEB_SERVER_CONTROL_TX_KEY_TIME_LIMIT_SECS 10

#define FEATURE_INTERNET_LINK_MAX_LINKS 2
#define FEATURE_INTERNET_LINK_DEFAULT_RCV_UDP_PORT 8888
#define FEATURE_INTERNET_LINK_BUFFER_TIME_MS 500
#define FEATURE_INTERNET_LINK_SVC_DURING_LOOP_TIME_MS 500 //SP5IOU 20220130 bug fix for FEATURE_INTERNET_LINK
#define FEATURE_INTERNET_LINK_KEY_DOWN_TIMEOUT_SECS 1  //SP5IOU 20220130 bug fix for FEATURE_INTERNET_LINK

#if defined(FEATURE_4x4_KEYPAD)|| defined (FEATURE_3x4_KEYPAD)
  #define KEYPAD_ROWS 4 //KeyPad Rows
  #if defined(FEATURE_4x4_KEYPAD)
    #define KEYPAD_COLS 4 //keypad Columns
  #else
    #define KEYPAD_COLS 3
  #endif
  #define mem1 0 //Memory numbers for Keypad: Actual memory numbers start with "0"
  #define mem2 1
  #define mem3 2
  #define mem4 3
  #define mem5 4
  #define mem6 5
  #define mem7 6
  #define mem8 7
  #define mem9 8
  #define mem10 9
  #define mem11 10
  #define mem12 11
#endif //#if defined(FEATURE_4x4_KEYPAD)|| defined (FEATURE_3x4_KEYPAD)

#define initial_sidetone_mode 1            // Sidetone mode, 0=OFF, 1=ON, 2=PADDLE_ONLY

#define sd_card_spi_ss_line 4

#if defined(OPTION_DFROBOT_LCD_COMMAND_BUTTONS)

  // For V1.1 board use these values
  #define dfrobot_btnRIGHT_analog 50
  #define dfrobot_btnUP_analog 250
  #define dfrobot_btnDOWN_analog 450
  #define dfrobot_btnLEFT_analog 650
  #define dfrobot_btnSELECT_analog 850  

  // For V1.0 board use these values
  // #define dfrobot_btnRIGHT_analog 50
  // #define dfrobot_btnUP_analog 195
  // #define dfrobot_btnDOWN_analog 380
  // #define dfrobot_btnLEFT_analog 555
  // #define dfrobot_btnSELECT_analog 790  
  
  // button to memory mappings (0 = command button, 1 = memory 1, 2 = memory 2, etc.)
  #define dfrobot_btnRIGHT  2
  #define dfrobot_btnUP     1
  #define dfrobot_btnDOWN   3
  #define dfrobot_btnLEFT   4
  #define dfrobot_btnSELECT 0
  #define dfrobot_btnNONE   255 // do not change

#endif

#if defined (FEATURE_LCD_LIQUIDCRYSTAL_I2C) || defined (TFT_HOSYOND_320x480_LCD) || defined(TFT_320x480_CAP_LCD) //|| defined(FEATURE_TFT7789_3_2inch_240x320_LCD)  // For TFT with touch controller         // for K7MDL version on ESP32-WROOM32 using esp-idf, tested on pins 21/22 i2c pins and a 4x20 display
  #define tx_key_line_active_state HIGH
  #define tx_key_line_inactive_state LOW
#else
  #define tx_key_line_active_state LOW
  #define tx_key_line_inactive_state HIGH
#endif


#if defined(FEATURE_TFT7789_3_2inch_240x320_LCD) || defined(TFT_HOSYOND_320x480_LCD) || defined(TFT_320x480_CAP_LCD)  // For TFT with touch controller
  #define bt_keyboard_LED_pin_active_state LOW
  #define bt_keyboard_LED_pin_inactive_state HIGH
#else
  #define bt_keyboard_LED_pin_active_state HIGH
  #define bt_keyboard_LED_pin_inactive_state LOW  
#endif

#define audio_enable_active_state LOW
#define audio_enable_inactive_state HIGH
#define sidetone_line_active_state HIGH
#define sidetone_line_inactive_state LOW
#define sequencer_pins_active_state HIGH
#define sequencer_pins_inactive_state LOW
#define ptt_line_active_state LOW
#define ptt_line_inactive_state HIGH

#define ptt_input_pin_active_state LOW
#define ptt_input_pin_inactive_state HIGH
#define tx_inhibit_pin_active_state LOW
#define tx_inhibit_pin_inactive_state HIGH
#define tx_pause_pin_active_state LOW
#define tx_pause_pin_inactive_state HIGH

#if defined(ARDUINO_MAPLE_MINI) || defined(ARDUINO_GENERIC_STM32f103C) || defined HARDWARE_ESP32_DEV//sp5iou 20180329, 20210825
  #define button_value_factor 4095  //sp5iou contributed
#else
  #define button_value_factor 1023
#endif

#define farnsworth_timing_calibration 1.15

#define sidetone_volume_low_limit 10
#define sidetone_volume_high_limit 500

#define custom_startup_field CODE_VERSION "K7MDL" //"your custom text here"   // an example could be callsign and name, eg. "AB1XYZ Bob", (or "Worlds best operator" which requires a 20 column display), string length shouldo be no more than the number of columns on the display

#define command_mode_acknowledgement_character 'E'

#if defined(FEATURE_COMMAND_MODE_ENHANCED_CMD_ACKNOWLEDGEMENT)
  #define command_a_iambic_a "A"
  #define command_b_iambic_b "B"
  #define command_c_single_paddle "SINGLE"
  #define command_d_ultimatic "ULT"
  #define command_h_weight_dit_dah_ratio_default "R"
  #define command_i_tx_on  "TX ON"
  #define command_i_tx_off "TX OFF"
  #define command_k_dit_dah_buffers_on "ON"
  #define command_k_dit_dah_buffers_off "OFF"
  #define command_n_paddle_reverse "REV"
  #define command_n_paddle_normal "NORM"
  #define command_o_sidetone_off "ST OFF"
  #define command_o_sidetone_paddle_only "ST PD ONLY"
  #define command_o_sidetone_on "ST ON"
  #define command_t_tune_mode "TUNE"
  #define command_v_potentiometer_on "POT ON"
  #define command_v_potentiometer_off "POT OFF"

  #define command_error "ERR"

#endif //FEATURE_COMMAND_MODE_ENHANCED_CMD_ACKNOWLEDGEMENT

#define HI_TEXT "K3NG"  // Must be in UPPER case
#define GRID_MEMORY 9   // this is where the user access the 4 or 6 digit grid square
// these are used when manually programming the memories (Ctrl-F11/12)
#define GRID_WORKING_MEMORY 11  //  Stored full 8 digit manually entered into memory 11 to override EEPROM and Config file
#define DECLINATION_WORKING_MEMORY 12  // stores manually entered declination to override config EEPROM and config file.

//  These are for BT Keyboard.  THE BT keys are translated to match the PS2 key codes
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
