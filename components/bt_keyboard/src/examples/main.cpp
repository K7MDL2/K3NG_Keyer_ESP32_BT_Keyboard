// Copyright (c) 2020 Guy Turcotte
//
// MIT License. Look at file licenses.txt for details.

#include <iostream>

#include "bt_keyboard.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

static constexpr char const *TAG = "Main";
char ch;
char ch_digit;
BTKeyboard bt_keyboard;

void pairing_handler(uint32_t pid) {
  std::cout << "Please enter the following pairing code, " << std::endl
            << "followed with ENTER on your keyboard: " << std::dec << pid << std::endl;
}

void keyboard_lost_connection_handler() {
  ESP_LOGW(TAG, "====> Lost connection with keyboard <====");
}

void keyboard_connected_handler() { ESP_LOGI(TAG, "----> Connected to keyboard <----"); }

extern "C" {

void app_main() {
  esp_err_t ret;

  // To test the Pairing code entry, uncomment the following line as pairing info is
  // kept in the nvs. Pairing will then be required on every boot.
  // ESP_ERROR_CHECK(nvs_flash_erase());

  ret = nvs_flash_init();
  if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "BT and BLE device Scan Setup");
  
  if (bt_keyboard.setup(pairing_handler, keyboard_connected_handler,
                        keyboard_lost_connection_handler)) { // Must be called once
    bt_keyboard.devices_scan(); // Required to discover new keyboards and for pairing
                                // Default duration is 5 seconds

    while (true) {
#if 0 // 0 = scan codes retrieval, 1 = augmented ASCII retrieval
          uint8_t ch = bt_keyboard.wait_for_ascii_char();
          // uint8_t ch = bt_keyboard.get_ascii_char(); // Without waiting

          if ((ch >= ' ') && (ch < 127)) std::cout << ch << std::flush; 
          else if (ch > 0) {
            std::cout << '[' << +ch << ']' << std::flush;
          }
#else
      BTKeyboard::KeyInfo inf;

      bt_keyboard.wait_for_low_event(inf);

      // simple decoding fdor Rii K08 BLE mini keyboard, aka Rii model i8+
      //std::cout << "RECEIVED KEYBOARD EVENT: ";
      //for (int n = 0; n < inf.size; n++) {
      if (inf.size == 8) {   // keyboard chars are len = 8, mousr and others are len=4
        if (inf.keys[2] != 0) {  // filter out key up events where are all 0s.
          std::cout << std::hex << +inf.keys[2] << ", ";  // do not print key up events
        }
      }
      
      if (inf.keys[2] < 0x1e) {
        ch = inf.keys[2]+93;  // if letter convert to 'a' and shift to upper case
        if (isalpha(ch)) {
          if (islower(ch)) 
            ch = toupper(ch) ;
        }
      }
      if (inf.keys[2] > 0x1d) {
        ch_digit = inf.keys[2]+ 0x13;  // if number 1, convert 0x1e to 0x30 for char '1'
        if (isdigit(ch_digit)) {
          ch = ch_digit;
        }
      }

      // filter out key up events
      if (inf.keys[2] != 0)
        std::cout << ch;

      std::cout << std::endl;
#endif
    }
  }
}
}