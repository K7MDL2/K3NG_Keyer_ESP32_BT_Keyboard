// Arduino-Pico's default "Pico SDK" USB stack, not "Adafruit TinyUSB".
void composite_begin();
void composite_service();

void setup() {
  Serial.begin(115200);
  composite_begin();
}

void loop() {
  composite_service();
}
