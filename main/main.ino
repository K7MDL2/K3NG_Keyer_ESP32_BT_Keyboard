// This is a shim to allow Arduino IDE to compile the program with the esp-idf version of main.cpp
// This shoudl not be modified.

#include <Arduino.h>

#ifndef PROJECT_ESP32_COMPILER

void setup_esp(); // call actual setup
void call_app_main();

void setup() {
  setup_esp(); // call actual setup
}

void loop() {
  call_app_main();
}

#endif