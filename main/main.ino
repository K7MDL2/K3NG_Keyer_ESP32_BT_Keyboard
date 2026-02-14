// This is a shim to allow Arduino IDE to compile the program with the esp-idf version of main.cpp
// This shoudl not be modified.

#include <Arduino.h>

#ifndef PROJECT_ESP32_COMPILER

void setup_esp(); // call actual setup
void call_app_main();

void setup() {
  Serial.begin(115200);
  delay(100);
  //Serial.println("Start Setup");
  setup_esp(); // call actual setup
}

void loop() {  
  //Serial.println("Start Main Loop");
  call_app_main();
}

#endif