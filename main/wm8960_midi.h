#ifndef WM8960_MIDI_H
#define WM8960_MIDI_H

#include <stdint.h>
#include <Arduino.h>

enum class Wm8960MidiPttMode : uint8_t { Off, OnOff, Toggle };
enum class Wm8960MidiControlMode : uint8_t { Local, Report, Pod };
// PC-side controls accepted in POD mode (CC 7, 23, 21, 22, 3 on the keyer channel).
enum class Wm8960MidiHostControl : uint8_t {
  MasterVolume, MicLevel, Speed, SidetoneLevel, SidetoneFrequency, Count };

void wm8960_midi_initialize();
void wm8960_midi_service(uint16_t sidetone_frequency, uint32_t speed_wpm,
                         uint8_t master_volume, uint8_t sidetone_volume,
                         uint8_t microphone_volume);
void wm8960_midi_key(bool down);
void wm8960_midi_ptt(bool down);
void wm8960_midi_ptt_from_host(bool down);
bool wm8960_midi_take_host_mic_level(uint8_t *percent, uint8_t *value);
// Returns the decoded value: percent for levels, WPM for speed, Hz for frequency.
bool wm8960_midi_take_host_control(Wm8960MidiHostControl control, uint16_t *value);
bool wm8960_midi_set_ptt_mode(uint8_t mode);
const char *wm8960_midi_ptt_mode_name(uint8_t mode);
bool wm8960_midi_set_control_mode(uint8_t mode);
Wm8960MidiControlMode wm8960_midi_control_mode();
const char *wm8960_midi_control_mode_name(uint8_t mode);
bool wm8960_midi_wheel_step(int direction);
void wm8960_midi_print_status(HardwareSerial &port);

#endif
