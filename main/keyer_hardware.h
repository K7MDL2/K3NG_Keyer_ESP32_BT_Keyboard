#ifndef KEYER_HARDWARE_H
#define KEYER_HARDWARE_H

// CPU families retained by this fork: ESP32, RP2040/RP2350, and Teensy 4.x.
#if (defined(PICO_PLATFORM) || defined(ARDUINO_ARCH_RP2040)) && !defined(ARDUINO_ESP32_DEV)
  #if defined(PICO_CYW43_SUPPORTED)
    #define ARDUINO_RASPBERRY_PI_PICO_W
  #else
    #define ARDUINO_RASPBERRY_PI_PICO
  #endif
#endif

#if defined(PROJECT_ESP32_COMPILER) || defined(ARDUINO_ESP32_DEV) || defined(ARDUINO_ARCH_ESP32)
  #define HARDWARE_ESP32_DEV
#endif

#if defined(TEENSYDUINO) && defined(__IMXRT1062__)
  #define PRIMARY_SERIAL_CLS usb_serial_class
  #define SECONDARY_SERIAL_CLS usb_serial_class
#elif defined(HARDWARE_ESP32_DEV)
  #define PRIMARY_SERIAL_CLS HardwareSerial
  #define SECONDARY_SERIAL_CLS HardwareSerial
  #define GPS_SERIAL_CLS HardwareSerial
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO)
  #define PRIMARY_SERIAL_CLS HardwareSerial
  #define SECONDARY_SERIAL_CLS HardwareSerial
  #define GPS_SERIAL_CLS SerialUART
#else
  #error "Unsupported keyer CPU: select ESP32, RP2040/RP2350, or Teensy 4.0/4.1"
#endif

// RP2350 Errata E10 reserves the last flash sector; keep the existing BT layout.
#if defined(ARDUINO_RASPBERRY_PI_PICO) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
  #ifndef PICO_FLASH_BANK_TOTAL_SIZE
    #define PICO_FLASH_BANK_TOTAL_SIZE (FLASH_SECTOR_SIZE * 2u)
  #endif
  #ifndef PICO_FLASH_BANK_STORAGE_OFFSET
    #define PICO_FLASH_BANK_STORAGE_OFFSET (PICO_FLASH_SIZE_BYTES - PICO_FLASH_BANK_TOTAL_SIZE)
  #endif
#endif

#endif
