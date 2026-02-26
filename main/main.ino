// This is a shim to allow Arduino IDE to compile the program with the esp-idf version of main.cpp
// This shoudl not be modified.

#include <Arduino.h>

#ifndef PROJECT_ESP32_COMPILER

  void setup_esp(); // call actual setup
  void call_app_main();
  extern void setup_1();
  extern void loop_1();

  #ifdef PICO_PLATFORM
    void setup1() {
      vTaskDelay(3200);  // long delay while we wait for main setup terminal to be ready
      Serial.println("Calling Setup_1 on Core 1");
      setup_1();
    }

    void loop1() {
      vTaskDelay(100); // only called when the above is commented out for test
      //Serial.println("Calling Loop_1 on Core 1");
      loop_1();  // never return     
    }
  #endif

  void setup() {
    Serial.begin(115200);
    vTaskDelay(3000);  // long delay to give time for the serial console to reset and read chars.
    Serial.println("\n\n*** Call Main Setup ***");
    setup_esp(); // call actual setup
  }

  void loop() {  
    vTaskDelay(100);
    Serial.println("Call App Main");
    call_app_main();  // will never return
  }

#endif