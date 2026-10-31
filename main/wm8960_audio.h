#ifndef WM8960_AUDIO_H
#define WM8960_AUDIO_H

#include <stdint.h>
#include <Arduino.h>

class Wm8960TerminalSerial : public HardwareSerial {
 public:
  void begin(unsigned long baud = 115200) override;
  void begin(unsigned long baud, uint16_t config) override;
  void end() override;
  int available() override;
  int availableForWrite() override;
  int peek() override;
  int read() override;
  void flush() override;
  size_t write(uint8_t value) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  using Print::write;
  operator bool() override;

 private:
  bool running_ = false;
};

extern Wm8960TerminalSerial Wm8960Terminal;
HardwareSerial &wm8960_audio_debug_port();

bool wm8960_audio_initialize(void (*report_progress)(const char *) = nullptr);
void wm8960_audio_service(void);
void wm8960_audio_start_tone(uint16_t frequency, uint32_t duration_ms,
                             uint32_t mute_release_ms);
void wm8960_audio_stop_tone(void);
void wm8960_audio_set_midi_input(bool keyed, bool ptt, uint16_t frequency);
void wm8960_audio_trace_keyer(uint8_t stage);
void wm8960_audio_trace_key_edge(bool down, uint8_t element_dits,
                                 uint16_t wpm);
bool wm8960_audio_set_levels(int sidetone, int playback, int microphone,
                            bool sidetone_muted, bool playback_muted,
                            bool microphone_muted);

#endif
