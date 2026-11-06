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
enum class Wm8960OutputRoute : uint8_t { Speaker, Headphone, Both, None };
bool wm8960_audio_set_output_route(Wm8960OutputRoute route);
Wm8960OutputRoute wm8960_audio_output_route();
const char *wm8960_audio_output_route_name();
struct Wm8960RxStatus {
  unsigned threshold, frequency, fixed_wpm, wpm, channel;
  float signal, noise;
  uint32_t dropped;
  uint32_t pending_samples, max_service_gap_ms, max_process_us;
  uint32_t max_foreground_us;
  uint8_t max_foreground_stage;
  unsigned tracked_frequency;
  uint32_t frequency_switches;
};
bool wm8960_audio_configure_rx(unsigned threshold, unsigned frequency,
                               unsigned wpm, unsigned channel);
Wm8960RxStatus wm8960_audio_rx_status();
void wm8960_audio_service_rx(void (*emit)(long));
void wm8960_audio_service(void);
void wm8960_audio_start_tone(uint16_t frequency, uint32_t duration_ms,
                             uint32_t mute_release_ms);
void wm8960_audio_stop_tone(void);
bool wm8960_audio_tone_active();
bool wm8960_audio_feedback_pending();
void wm8960_audio_set_local_ptt(bool active);
void wm8960_audio_set_external_ptt(bool active);
bool wm8960_audio_external_ptt();
void wm8960_audio_set_midi_input(bool keyed, bool ptt, uint16_t frequency);
void wm8960_audio_pause_for_flash();
void wm8960_audio_i2c_test(Print &port, uint16_t count, uint8_t expander_address);
void wm8960_audio_resume_after_flash();
void wm8960_audio_trace_keyer(uint8_t stage);
void wm8960_audio_trace_key_edge(bool down, uint8_t element_dits,
                                 uint16_t wpm);
bool wm8960_audio_set_levels(int sidetone, int playback, int microphone,
                            bool sidetone_muted, bool playback_muted,
                            bool microphone_muted);
bool wm8960_audio_set_mic_gain(uint8_t pga, uint8_t boost, bool alc);
// Sets the gain that wm8960_audio_initialize() will apply; fails once the codec is running.
bool wm8960_audio_preset_mic_gain(uint8_t pga, uint8_t boost, bool alc);
void wm8960_audio_get_mic_gain(uint8_t *pga, uint8_t *boost, bool *alc);
void wm8960_audio_set_mic_test_tone(bool enabled);
bool wm8960_audio_mic_test_tone();
// Microphone PTT gate mode: 0=always, 1=local PTT, 2=external feedback, 3=either.
bool wm8960_audio_set_mic_gate_mode(uint8_t mode);
uint8_t wm8960_audio_mic_gate_mode();
void wm8960_audio_take_mic_meter(uint16_t *peak, uint16_t *rms,
                                 uint32_t *clips, uint32_t *samples);

#endif
