#include "keyer_hardware.h"
#include "keyer_hardware.h"
#include "keyer_diagnostics.h"
#include "keyer_dependencies.h"
#include "keyer_keyboard_codes.h"
#include "keyer_keyboard_codes.h"
#include <cassert>
#include <cstring>
#include <type_traits>

struct HardwareSerial {};
struct SerialUART {};
struct usb_serial_class {};

static_assert(BT_KEY_TAB == 9 && BT_KEY_ENTER == 13 && BT_KEY_ESC == 27);
static_assert(BT_KEY_BACKSPACE == 127 && BT_KEY_DELETE == 127 && BT_KEY_INSERT == 128);
static_assert(BT_KEY_HOME == 129 && BT_KEY_END == 156);
static_assert(BT_KEY_UPARROW == 11 && BT_KEY_DOWNARROW == 10);
static_assert(BT_KEY_LEFTARROW == 8 && BT_KEY_RIGHTARROW == 21);
static_assert(BT_KEY_F1 == 130 && BT_KEY_F12 == 141);
static_assert(BT_KEY_F1_SHIFT == 143 && BT_KEY_F12_SHIFT == 154);

#if defined(OPTION_PS2_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT) && !defined(OPTION_KEYBOARD_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT)
  #error "Legacy LCD keyboard option must select the renamed option"
#endif

#if defined(OPTION_BT_KEYBOARD_US) || defined(OPTION_BT_KEYBOARD_GERMAN) || defined(OPTION_BT_KEYBOARD_FRENCH)
constexpr int control_codes[] = {
    BT_KEY_A_CTRL, BT_KEY_B_CTRL, BT_KEY_C_CTRL, BT_KEY_D_CTRL, BT_KEY_E_CTRL,
    BT_KEY_F_CTRL, BT_KEY_G_CTRL, BT_KEY_H_CTRL, BT_KEY_I_CTRL, BT_KEY_J_CTRL,
    BT_KEY_K_CTRL, BT_KEY_L_CTRL, BT_KEY_M_CTRL, BT_KEY_N_CTRL, BT_KEY_O_CTRL,
    BT_KEY_P_CTRL, BT_KEY_Q_CTRL, BT_KEY_R_CTRL, BT_KEY_S_CTRL, BT_KEY_T_CTRL,
    BT_KEY_U_CTRL, BT_KEY_V_CTRL, BT_KEY_W_CTRL, BT_KEY_X_CTRL, BT_KEY_Y_CTRL, BT_KEY_Z_CTRL
};
constexpr int control_functions[] = {
    BT_KEY_F1_CTRL, BT_KEY_F2_CTRL, BT_KEY_F3_CTRL, BT_KEY_F4_CTRL,
    BT_KEY_F5_CTRL, BT_KEY_F6_CTRL, BT_KEY_F7_CTRL, BT_KEY_F8_CTRL,
    BT_KEY_F9_CTRL, BT_KEY_F10_CTRL, BT_KEY_F11_CTRL, BT_KEY_F12_CTRL
};
constexpr int alt_functions[] = {
    BT_KEY_F1_ALT, BT_KEY_F2_ALT, BT_KEY_F3_ALT, BT_KEY_F4_ALT,
    BT_KEY_F5_ALT, BT_KEY_F6_ALT, BT_KEY_F7_ALT, BT_KEY_F8_ALT,
    BT_KEY_F9_ALT, BT_KEY_F10_ALT, BT_KEY_F11_ALT, BT_KEY_F12_ALT
};
#endif

#if defined(TEST_ESP32)
  #ifndef HARDWARE_ESP32_DEV
    #error "ESP32 profile not selected"
  #endif
  static_assert(std::is_same<PRIMARY_SERIAL_CLS, HardwareSerial>::value);
  static_assert(std::is_same<SECONDARY_SERIAL_CLS, HardwareSerial>::value);
  static_assert(std::is_same<GPS_SERIAL_CLS, HardwareSerial>::value);
#elif defined(TEST_PICO)
  #if defined(TEST_WIRELESS)
    #ifndef ARDUINO_RASPBERRY_PI_PICO_W
      #error "Wireless Pico profile not selected"
    #endif
  #else
    #ifndef ARDUINO_RASPBERRY_PI_PICO
      #error "Wired Pico profile not selected"
    #endif
  #endif
  static_assert(std::is_same<PRIMARY_SERIAL_CLS, HardwareSerial>::value);
  static_assert(std::is_same<SECONDARY_SERIAL_CLS, HardwareSerial>::value);
  static_assert(std::is_same<GPS_SERIAL_CLS, SerialUART>::value);
  static_assert(PICO_FLASH_BANK_TOTAL_SIZE == 8192);
  static_assert(PICO_FLASH_BANK_STORAGE_OFFSET == 4194304 - 8192);
#elif defined(TEST_TEENSY)
  static_assert(std::is_same<PRIMARY_SERIAL_CLS, usb_serial_class>::value);
  static_assert(std::is_same<SECONDARY_SERIAL_CLS, usb_serial_class>::value);
#endif

int main() {
#if defined(OPTION_BT_KEYBOARD_US) || defined(OPTION_BT_KEYBOARD_GERMAN) || defined(OPTION_BT_KEYBOARD_FRENCH)
  int expected_control[26];
  for (int i = 0; i < 26; ++i) expected_control[i] = 162 + i;
  #if defined(OPTION_BT_KEYBOARD_GERMAN)
    expected_control[2] = 200; expected_control[5] = 201; expected_control[14] = 202;
    expected_control[16] = 203; expected_control[17] = 204; expected_control[19] = 205;
  #elif defined(OPTION_BT_KEYBOARD_FRENCH)
    expected_control[1] = 200; expected_control[5] = 201; expected_control[6] = 202;
    expected_control[14] = 203; expected_control[16] = 204; expected_control[19] = 205;
  #endif
  for (int i = 0; i < 26; ++i) assert(control_codes[i] == expected_control[i]);
  for (int i = 0; i < 12; ++i) {
    int expected_function = 188 + i;
    #if defined(OPTION_BT_KEYBOARD_GERMAN)
      if (i == 8) expected_function = 206;
      assert(alt_functions[i] == 229 + i);
    #elif defined(OPTION_BT_KEYBOARD_FRENCH)
      assert(alt_functions[i] == 233 + i);
    #else
      assert(alt_functions[i] == 200 + i);
    #endif
    assert(control_functions[i] == expected_function);
  }
#endif
  const char *names[] = {"keyer/buttons", "serial", "touch", "Bluetooth keyboard",
      "keyboard/GPS", "settings/memories", "display", "other foreground",
      "BOOTSEL", "loop/audio", "send buffer/PTT", "potentiometer/encoders"};
  for (unsigned stage = 0; stage <= 255; ++stage) {
    const char *expected = stage >= 9 && stage <= 20 ? names[stage - 9] : "none";
    assert(std::strcmp(keyer_foreground_stage_name(static_cast<uint8_t>(stage)), expected) == 0);
  }
}
