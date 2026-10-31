#ifndef WM8960_MIDI_H
#define WM8960_MIDI_H

#include <stdint.h>
#include <Arduino.h>

enum class Wm8960MidiPttMode : uint8_t { Off, OnOff, Toggle };

void wm8960_midi_initialize();
void wm8960_midi_service(uint16_t sidetone_frequency, uint32_t speed_wpm);
void wm8960_midi_key(bool down);
void wm8960_midi_ptt(bool down);
bool wm8960_midi_set_ptt_mode(uint8_t mode);
const char *wm8960_midi_ptt_mode_name(uint8_t mode);
bool wm8960_midi_wheel_step(int direction);
void wm8960_midi_print_status(HardwareSerial &port);

#endif
