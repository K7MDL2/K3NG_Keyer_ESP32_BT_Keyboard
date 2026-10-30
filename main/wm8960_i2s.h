#ifndef WM8960_I2S_H
#define WM8960_I2S_H

#include <Arduino.h>

namespace wm8960_i2s {
bool begin(uint32_t sample_rate);
int available();
size_t read(uint8_t *buffer, size_t bytes);
int available_for_write();
size_t write(const uint8_t *buffer, size_t bytes);
uint32_t received_frames();
uint32_t transmitted_frames();
uint32_t underruns();
uint32_t max_service_gap_us();
uint32_t queued_frames();
}

#endif
