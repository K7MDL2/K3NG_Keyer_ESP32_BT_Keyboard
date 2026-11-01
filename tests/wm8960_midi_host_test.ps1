$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$midi = Get-Content (Join-Path $root 'main\wm8960_midi.cpp') -Raw
$header = Get-Content (Join-Path $root 'main\wm8960_midi.h') -Raw
$audio = Get-Content (Join-Path $root 'main\wm8960_audio.cpp') -Raw
$main = Get-Content (Join-Path $root 'main\main.cpp') -Raw
$midi = [regex]::Replace($midi, '(?m)^#include .*\r?$', '')
$header = [regex]::Replace($header, '(?m)^#include .*\r?$', '')

function Get-CppFunction([string]$text, [string]$name) {
    $match = [regex]::Match($text, "(?m)^(?:bool|void|int|int16_t|uint16_t) $name\([^;]*?\)\s*\{")
    if (!$match.Success) { throw "Function not found: $name" }
    $start = $match.Index
    $opening = $text.IndexOf('{', $start)
    $depth = 1
    $end = $opening + 1
    while ($depth -gt 0 -and $end -lt $text.Length) {
        if ($text[$end] -eq '{') { $depth++ }
        if ($text[$end] -eq '}') { $depth-- }
        $end++
    }
    if ($depth -ne 0) { throw "Unbalanced function: $name" }
    return $text.Substring($start, $end - $start)
}

$audioFunctions = @(
    'advance_mute_ramp', 'apply_mute_ramp', 'usb_audio_muted_for_keyer',
    'next_tone_sample', 'wm8960_audio_stop_tone',
    'wm8960_audio_set_midi_input', 'render_i2s_output'
) | ForEach-Object { Get-CppFunction $audio $_ }
$sine = [regex]::Match($audio, '(?s)constexpr int16_t kSineTable\[64\] = \{.*?\};').Value
if (!$sine) { throw 'Sine table not found' }
$config = [regex]::Match($main, '(?s)struct config_t \{.*?\} configuration;').Value
if (!$config) { throw 'EEPROM configuration structure not found' }
$cli = Get-CppFunction $main 'cli_midi'
$encoder = [regex]::Match($main, '(?s)enum class EncoderFunction.*?(?=\r?\n#endif\r?\n\r?\nint chk_rotary_encoder)').Value
if (!$encoder) { throw 'Encoder menu functions not found' }
$encoderCapture = [regex]::Match($main, '(?s)struct EncoderEvent.*?(?=\r?\n#endif)').Value
if (!$encoderCapture) { throw 'Encoder interrupt capture not found' }
$encoderTable = [regex]::Match($main, '(?s)#ifdef OPTION_ENCODER_HALF_STEP_MODE.*?(?=\r?\n#endif //FEATURE_ENCODER_SUPPORT)').Value
$settings = Get-Content (Join-Path $root 'main\keyer_settings_pico.h') -Raw
$encoderDirection = [regex]::Match($settings, '(?s)#ifndef ENCODER_VFO_DIRECTION.*?(?=\r?\n#define sd_card_spi_ss_line)').Value
if (!$encoderDirection) { throw 'Encoder VFO direction setting not found' }
$encoderFunctions = @('service_keyer_midi', 'audio_volume', 'step_audio_volume', 'set_audio_volume',
    'initialize_potentiometer', 'initialize_rotary_encoder', 'chk_rotary_encoder',
    'check_rotary_encoder') |
    ForEach-Object { Get-CppFunction $main $_ }
$potGuard = [regex]::Match($main, '(?s)#if defined\(potentiometer\) && potentiometer == 0.*?#endif').Value
$keyInterrupts = [regex]::Match($main, '(?s)static void left_paddle_intr_handler\(\).*?(?=\r?\n#endif // non ESP32 local pin interrupt setup)').Value
if (!$keyInterrupts) { throw 'Local key interrupt initialization not found' }

$mock = @'
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <cctype>
#include <string>
#include <vector>
#define ARDUINO_ARCH_RP2040
#define FEATURE_WM8960_USB_AUDIO
#define FEATURE_INTERNET_LINK_MAX_LINKS 2
#define F(text) text
#define PRIMARY_SERIAL_CLS HardwareSerial
#define FEATURE_DISPLAY
#define FEATURE_TFT_DISPLAY
#define FEATURE_ROTARY_ENCODER
#define OPTION_ENCODER_ENABLE_PULLUPS
#ifndef TEST_ENCODER_FULL_STEP
#define OPTION_ENCODER_HALF_STEP_MODE
#endif
#define FEATURE_POTENTIOMETER
#define potentiometer 0
#define rotary_pin1 2
#define rotary_pin2 3
#ifndef rotary_switch
#define rotary_switch 4
#endif
constexpr uint8_t grid_len_max = 9;
class String {
  std::string text;
 public:
  String(const char *value = "") : text(value) {}
  String(int value) : text(std::to_string(value)) {}
  String(unsigned int value) : text(std::to_string(value)) {}
  explicit String(std::string value) : text(value) {}
  void trim() {
    auto start = text.find_first_not_of(" \r\n\t");
    if (start == std::string::npos) { text.clear(); return; }
    text = text.substr(start, text.find_last_not_of(" \r\n\t") - start + 1);
  }
  int indexOf(char value) const {
    auto found = text.find(value);
    return found == std::string::npos ? -1 : static_cast<int>(found);
  }
  String substring(size_t start) const { return String(text.substr(start)); }
  String substring(size_t start, size_t end) const {
    return String(text.substr(start, end - start));
  }
  size_t length() const { return text.size(); }
  bool operator==(const char *value) const { return text == value; }
  bool operator!=(const char *value) const { return text != value; }
  friend String operator+(const String &left, const String &right) {
    return String(left.text + right.text);
  }
  const std::string &str() const { return text; }
};
uint8_t config_dirty = 0;
template<class T> T constrain(T value, int lo, int hi) {
  return std::max<T>(static_cast<T>(lo), std::min<T>(static_cast<T>(hi), value));
}
uint32_t clock_ms = 100, clock_us = 100000;
uint32_t millis() { return clock_ms; }
uint32_t micros() { return clock_us; }
void advance(uint32_t ms) { clock_ms += ms; clock_us += ms * 1000; }
int get_core_num() { return 0; }
uint32_t save_and_disable_interrupts() { return 0; }
void restore_interrupts(uint32_t) {}
bool mounted = true, suspended = false, writable = true, lockable = true;
int write_budget = -1;
struct { int mutex; } USB;
struct CoreMutex {
  bool acquired;
  CoreMutex(int *, bool) : acquired(lockable) {}
  operator bool() const { return acquired; }
};
struct HardwareSerial {
  std::string output;
  bool open = false;
  int write_space = 512;
  operator bool() const { return open; }
  int availableForWrite() const { return write_space; }
  size_t write(const uint8_t *bytes, size_t count) {
    output.append(reinterpret_cast<const char *>(bytes), count); return count;
  }
  void print(const char *text) { output += text; }
  void println(const char *text) { output += text; output += '\n'; }
  void printf(const char *format, ...) {
    char text[512]; va_list args; va_start(args, format);
    vsnprintf(text, sizeof(text), format, args); va_end(args); output += text;
  }
} terminal;
HardwareSerial &wm8960_audio_debug_port() { return terminal; }
std::deque<std::array<uint8_t,4>> incoming;
std::vector<std::array<uint8_t,4>> outgoing;
bool tud_mounted() { return mounted; }
bool tud_suspended() { return suspended; }
bool tud_midi_mounted() { return mounted; }
bool tud_midi_packet_write(const uint8_t *packet) {
  if (!writable || write_budget == 0) return false;
  if (write_budget > 0) --write_budget;
  outgoing.push_back({packet[0],packet[1],packet[2],packet[3]}); return true;
}
bool tud_midi_packet_read(uint8_t *packet) {
  if (incoming.empty()) return false;
  memcpy(packet, incoming.front().data(), 4); incoming.pop_front(); return true;
}
constexpr uint32_t kSampleRate = 48000;
constexpr uint16_t kMuteRampFrames = 240, kPlaybackRingFrames = 1024;
constexpr int16_t kToneAmplitude = 8192;
bool codec_ready = true;
volatile bool tone_active = false, tone_mute_pending = false;
volatile bool midi_tone_active = false, midi_ptt_active = false;
volatile uint32_t tone_stopped_ms = 0, tone_mute_release_ms = 0;
uint32_t tone_phase = 0, tone_phase_increment = 0, tone_frames_remaining = 0;
uint32_t midi_tone_phase = 0, midi_tone_phase_increment = 0, tone_output_frames = 0;
uint32_t playback_read_index = 0, playback_write_index = 0;
uint32_t playback_starved_frames = 0;
bool playback_started = false;
uint16_t playback_mute_level = kMuteRampFrames;
int32_t local_sidetone_gain_q15 = 32767, local_playback_gain_q15 = 32767;
int16_t playback_ring[kPlaybackRingFrames * 2] = {};
namespace wm8960_i2s {
  std::vector<int16_t> output;
  int available_for_write() { return 512; }
  size_t write(const uint8_t *bytes, size_t count) {
    output.resize(count / 2); memcpy(output.data(), bytes, count); return count;
  }
}
'@

$encoderMocks = @'
enum class AudioVolumeTarget : uint8_t { Playback, Sidetone, Microphone };
int audio_volume(AudioVolumeTarget);
void service_keyer_midi();
int step_audio_volume(int, bool);
void set_audio_volume(AudioVolumeTarget, int, bool);
template<class T> T max(T a, T b) { return std::max(a,b); }
template<class T> T min(T a, T b) { return std::min(a,b); }
bool apply_audio_levels() { return true; }
constexpr int LOW = 0, HIGH = 1, INPUT = 0, INPUT_PULLUP = 2;
constexpr int KEYER_COMMAND_MODE = 1;
#define FEATURE_STRAIGHT_KEY
constexpr int paddle_left = 30, paddle_right = 31, pin_straight_key = 4;
bool paddle_left_state = true, paddle_right_state = true, straight_key_state = true;
int keyer_machine_mode = 0;
int pin_values[48] = {};
constexpr int CHANGE = 3;
std::vector<int> interrupt_pins;
void (*encoder_irq)() = nullptr;
int digitalPinToInterrupt(int pin) { return pin; }
void attachInterrupt(int pin, void (*handler)(), int mode) {
  assert(mode == CHANGE); interrupt_pins.push_back(pin); encoder_irq = handler;
}
HardwareSerial *debug_serial_port = &terminal;
std::vector<std::array<int,2>> pin_modes;
int digitalRead(int pin) { assert(pin != 0); return pin_values[pin]; }
void pinMode(int pin, int mode) {
  assert(pin != 0); pin_modes.push_back({pin,mode});
}
void digitalWrite(int, int) { assert(false); }
std::string lcd_text;
constexpr int TFT_BLACK = 0, ICON_ANCHOR = 250, STATUS_BAR_FONT = 2;
struct MockLcd {
  int draws = 0;
  std::string label;
  uint16_t color565(int r, int g, int b) {
    assert(r == 144 && g == 238 && b == 144); return 0x9772;
  }
  void setTextColor(int fg, int bg) { assert(fg == 0x9772 && bg == TFT_BLACK); }
  void drawString(const char *text, int x, int row, int font) {
    assert(x == ICON_ANCHOR && row == 6 && font == STATUS_BAR_FONT);
    label = text; ++draws;
  }
} lcd;
void lcd_center_print_timed(String text, int, int) { lcd_text = text.str(); }
void speed_change(int step) { configuration.wpm += step; config_dirty = 1; }
void speed_change_command_mode(int step) {
  configuration.wpm_command_mode += step; config_dirty = 1;
}
'@

$checks = @'
void wm8960_midi_service(uint16_t frequency, uint32_t speed) {
  wm8960_midi_service(frequency, speed, 100, 50, 100);
}
void fresh() {
  mounted = writable = lockable = true; suspended = false;
  write_budget = -1;
  reset_session(); connected = false; local_key = local_ptt = false;
  ptt_mode = Wm8960MidiPttMode::Off;
  incoming.clear(); outgoing.clear(); terminal.output.clear();
  tone_active = tone_mute_pending = false;
  wm8960_midi_initialize(); wm8960_midi_service(750, 20);
  outgoing.clear();
}
void service() { wm8960_midi_service(750, 20); }
void receive(uint8_t status, uint8_t note, uint8_t value, uint8_t cin = 9) {
  advance(20); incoming.push_back({cin,status,note,value}); service();
}
size_t messages(uint8_t status, uint8_t data, uint8_t value) {
  return std::count_if(outgoing.begin(), outgoing.end(), [&](const auto &p) {
    return p[1] == status && p[2] == data && p[3] == value;
  });
}
int main() {
  fresh();
  wm8960_midi_key(true); wm8960_midi_key(true); service();
  wm8960_midi_key(false); wm8960_midi_key(false); service();
  assert(messages(0x99,17,127) == 1 && messages(0x99,17,0) == 1);
  wm8960_midi_ptt(true); wm8960_midi_ptt(false); service();
  assert(messages(0x99,18,127) == 0);
  for (uint8_t mode : {1,2}) {
    fresh(); assert(wm8960_midi_set_ptt_mode(mode));
    wm8960_midi_ptt(true); service();
    assert(!wm8960_midi_set_ptt_mode(0));
    wm8960_midi_ptt(false); service();
    assert(messages(0x99,18,127) == (mode == 2 ? 2 : 1));
    assert(messages(0x99,18,0) == (mode == 1 ? 1 : 0));
    assert(wm8960_midi_set_ptt_mode(0));
  }
  assert(!wm8960_midi_set_ptt_mode(3));
  fresh();
  assert(wm8960_midi_wheel_step(1)); assert(wm8960_midi_wheel_step(-1));
  assert(wm8960_midi_wheel_step(0)); service();
  assert(messages(0xB9,20,127) == 1 && messages(0xB9,20,1) == 1);
  fresh(); writable = false;
  wm8960_midi_key(true); service(); assert(outgoing.empty());
  wm8960_midi_key(false); writable = true; advance(10); service();
  assert(messages(0x99,17,127) == 1 && messages(0x99,17,0) == 1);
  fresh(); lockable = false; wm8960_midi_key(true); service();
  assert(outgoing.empty()); lockable = true; service();
  assert(messages(0x99,17,127) == 1);
  fresh(); receive(0x99,17,127); assert(midi_tone_active && usb_audio_muted_for_keyer());
  receive(0x90,17,127); receive(0x99,17,0);
  assert(midi_tone_active);
  receive(0x80,17,0,8); assert(!midi_tone_active);
  receive(0x90,18,127); assert(midi_ptt_active && !midi_tone_active);
  receive(0x80,18,64,8); assert(!midi_ptt_active);
  receive(0x90,17,127); receive(0x90,18,127);
  receive(0xB0,123,0,11); assert(!midi_tone_active && !midi_ptt_active);
  receive(0x90,17,127); receive(0xB0,120,0,11); assert(!midi_tone_active);
  const auto invalid_before = invalid_packets;
  receive(0x90,17,128); receive(0x80,17,0,9);
  incoming.push_back({0x19,0x90,17,127}); service();
  assert(invalid_packets == invalid_before + 3 && !midi_tone_active);
  fresh(); wm8960_midi_key(true); service();
  const auto echoed = outgoing.back(); const auto size = outgoing.size();
  incoming.push_back(echoed); service();
  assert(outgoing.size() == size && !midi_tone_active);
  fresh(); receive(0x90,17,127);
  auto echo = outgoing.back(); const auto echo_size = outgoing.size();
  incoming.push_back(echo); service(); assert(outgoing.size() == echo_size);
  advance(20); incoming.push_back(echo); service();
  assert(outgoing.size() == echo_size + 1);
  fresh(); receive(0x90,17,127); tone_active = true;
  receive(0x90,17,0); assert(tone_active && usb_audio_muted_for_keyer());
  receive(0x90,17,127); wm8960_audio_stop_tone();
  assert(midi_tone_active && usb_audio_muted_for_keyer());
  tone_mute_pending = true; tone_stopped_ms = clock_ms; tone_mute_release_ms = 420;
  receive(0x90,17,0); assert(usb_audio_muted_for_keyer());
  advance(420); assert(!usb_audio_muted_for_keyer());
  fresh(); wm8960_audio_set_midi_input(true,false,750);
  render_i2s_output();
  assert(std::any_of(wm8960_i2s::output.begin(),wm8960_i2s::output.end(),
                    [](int16_t sample) { return sample != 0; }));
  wm8960_audio_set_midi_input(false,false,750); render_i2s_output();
  assert(std::all_of(wm8960_i2s::output.begin(),wm8960_i2s::output.end(),
                    [](int16_t sample) { return sample == 0; }));
  tone_phase_increment = uint32_t((uint64_t(600) << 32) / kSampleRate);
  tone_active = true; tone_frames_remaining = 32;
  wm8960_audio_set_midi_input(true,false,750); render_i2s_output();
  assert(!tone_active && midi_tone_active);
  assert(std::any_of(wm8960_i2s::output.begin()+64,wm8960_i2s::output.end(),
                    [](int16_t sample) { return sample != 0; }));
  fresh(); receive(0x90,17,127); writable = false; wm8960_midi_key(true);
  tud_umount_cb(); assert(!midi_tone_active);
  mounted = false; service(); assert(queue_count == 0);
  mounted = writable = true; service(); assert(messages(0x99,17,127) == 1);
  fresh(); receive(0x90,17,127); writable = false;
  wm8960_midi_key(true); tud_mount_cb();
  assert(!midi_tone_active); writable = true; outgoing.clear(); service();
  assert(queue_count == 0 && messages(0x99,17,127) == 1);
  fresh(); assert(wm8960_midi_set_ptt_mode(2)); wm8960_midi_ptt(true); service();
  tud_suspend_cb(false); suspended = true; service();
  suspended = false; service(); outgoing.clear();
  wm8960_midi_key(true); wm8960_midi_ptt(false); service();
  assert(messages(0x99,18,127) == 0);
  fresh(); assert(wm8960_midi_set_ptt_mode(2)); wm8960_midi_ptt(true); service();
  writable = false;
  for (int index = 0; index <= kQueueSize; ++index) wm8960_midi_wheel_step(1);
  assert(faulted && queue_count == 0);
  writable = true; service();
  assert(messages(0x99,17,0) == 1 && messages(0x99,18,127) == 2);
  assert(!cleanup_pending && !wm8960_midi_wheel_step(1));
  fresh(); assert(wm8960_midi_set_ptt_mode(1));
  wm8960_midi_ptt(true); service(); writable = false;
  for (int index = 0; index <= kQueueSize; ++index) wm8960_midi_wheel_step(1);
  writable = true; write_budget = 1; service();
  assert(cleanup_pending && !cleanup_key_pending);
  write_budget = 1; service();
  assert(!cleanup_pending && messages(0x99,18,0) == 1);
  fresh(); writable = false; wm8960_midi_key(true); service();
  advance(249); service(); assert(!faulted);
  advance(1); service(); assert(faulted && cleanup_pending);
  writable = true; service(); assert(messages(0x99,17,0) == 1);
  fresh(); clock_ms = UINT32_MAX - 100; clock_us = UINT32_MAX - 10000;
  writable = false; wm8960_midi_key(true); advance(200); service();
  assert(!faulted); advance(50); service(); assert(faulted);
  fresh();
  configuration.midi_ptt_mode = 0; config_dirty = 0;
  assert(cli_midi(&terminal, "MIDI-PTT ONOFF"));
  assert(configuration.midi_ptt_mode == 1 && config_dirty == 1);
  config_t saved = configuration;
  config_dirty = 0;
  assert(cli_midi(&terminal, "MIDI-PTT BOGUS"));
  assert(cli_midi(&terminal, "MIDI-PTT ONOFF EXTRA"));
  assert(memcmp(&saved, &configuration, sizeof(saved)) == 0 && !config_dirty);
  terminal.output = "\\:midi-ptt";
  assert(cli_midi(&terminal, "MIDI-PTT"));
  assert(terminal.output.find("\\:midi-ptt MIDI PTT ") == 0);
  terminal.output = "\\:midi";
  assert(cli_midi(&terminal, "MIDI"));
  assert(terminal.output.find("\\:midi MIDI PTT ") == 0);
  assert(!cli_midi(&terminal, "UNRELATED"));
  assert(cli_midi(&terminal, "MIDIWHEEL +"));
  assert(cli_midi(&terminal, "MIDIWHEEL -")); service();
  assert(messages(0xB9,20,127) == 1 && messages(0xB9,20,1) == 1);
  assert(cli_midi(&terminal, "MIDIWHEEL INVALID"));
  wm8960_midi_ptt(true); service();
  assert(cli_midi(&terminal, "MIDI-PTT OFF"));
  assert(configuration.midi_ptt_mode == 1);
  wm8960_midi_ptt(false); service();
  for (uint8_t mode : {0,1,2}) {
    configuration.midi_ptt_mode = mode; configuration.wpm = 23;
    configuration.sidetone_volume = 50; configuration.usb_play_volume = 65;
    configuration.usb_mic_volume = 80; configuration.usb_play_muted = true;
    std::array<uint8_t,sizeof(config_t)> eeprom = {};
    memcpy(eeprom.data(), &configuration, sizeof(configuration));
    config_t restored = {};
    memcpy(&restored, eeprom.data(), sizeof(restored));
    assert(restored.midi_ptt_mode == mode && restored.wpm == 23);
    assert(restored.sidetone_volume == 50 && restored.usb_play_volume == 65);
    assert(restored.usb_mic_volume == 80 && restored.usb_play_muted);
    assert(memcmp(&restored, &configuration, sizeof(restored)) == 0);
  }
  fresh();
  outgoing.clear(); wm8960_midi_service(300, 20);
  assert(messages(0xB9,3,0) == 1);
  outgoing.clear(); wm8960_midi_service(301, 20);
  assert(messages(0xB9,3,0) == 1); // Report even a change below one MIDI unit.
  outgoing.clear(); wm8960_midi_service(1000, 20);
  assert(messages(0xB9,3,127) == 1);
  outgoing.clear(); wm8960_midi_service(1500, 20);
  assert(messages(0xB9,3,127) == 0);
  advance(1000); wm8960_midi_service(1500, 20);
  assert(messages(0xB9,3,127) == 0);
  fresh();
  mounted = false; service(); outgoing.clear();
  mounted = true; service();
  assert(messages(0xB9,7,127) == 1 && messages(0xB9,22,64) == 1);
  assert(messages(0xB9,23,127) == 1 && messages(0xB9,3,82) == 1);
  outgoing.clear(); service(); assert(outgoing.empty());
  advance(300000); service();
  assert(outgoing.size() == 1 && messages(0xB9,21,41) == 1);
  for (uint8_t level = 0; level <= 100; ++level) {
    outgoing.clear(); wm8960_midi_service(750, 20, level, level, level);
    const uint8_t value = (level * 127u + 50u) / 100u;
    for (uint8_t control : {7,22,23}) {
      assert(messages(0xB9,control,value) == 1);
    }
    assert(outgoing.size() == 3);
    outgoing.clear(); wm8960_midi_service(750, 20, level, level, level);
    assert(outgoing.empty());
  }
  outgoing.clear(); wm8960_midi_service(750, 20, 255, 255, 255);
  assert(outgoing.empty()); // Out-of-range volumes clamp to 100%.
  // Backpressure and USB-lock contention must not lose control reports.
  fresh(); writable = false;
  wm8960_midi_service(600, 20, 0, 25, 75);
  assert(outgoing.empty() && queue_count == 4);
  writable = true; wm8960_midi_service(600, 20, 0, 25, 75);
  assert(messages(0xB9,7,0) == 1 && messages(0xB9,22,32) == 1);
  assert(messages(0xB9,23,95) == 1 && messages(0xB9,3,54) == 1);
  outgoing.clear(); lockable = false;
  wm8960_midi_service(700, 20, 80, 60, 40);
  assert(outgoing.empty());
  lockable = true; wm8960_midi_service(700, 20, 80, 60, 40);
  assert(messages(0xB9,7,102) == 1 && messages(0xB9,22,76) == 1);
  assert(messages(0xB9,23,51) == 1 && messages(0xB9,3,73) == 1);
  fresh();
  configuration.hz_sidetone = 750; configuration.wpm = 20;
  configuration.usb_play_volume = 80; configuration.sidetone_volume = 60;
  configuration.usb_mic_volume = 40;
  configuration.usb_play_muted = configuration.sidetone_muted =
      configuration.usb_mic_muted = true;
  service_keyer_midi();
  for (uint8_t control : {7,22,23}) assert(messages(0xB9,control,0) == 1);
  assert(configuration.usb_play_volume == 80 && configuration.sidetone_volume == 60 &&
         configuration.usb_mic_volume == 40);
  outgoing.clear();
  configuration.usb_play_muted = configuration.sidetone_muted =
      configuration.usb_mic_muted = false;
  service_keyer_midi();
  assert(messages(0xB9,7,102) == 1 && messages(0xB9,22,76) == 1);
  assert(messages(0xB9,23,51) == 1);
  fresh();
  // Connection publishes speed, unchanged speed waits exactly five minutes.
  mounted = false; service(); outgoing.clear();
  mounted = true; service();
  assert(messages(0xB9,21,41) == 1);
  outgoing.clear(); service(); assert(messages(0xB9,21,41) == 0);
  advance(299999); service(); assert(messages(0xB9,21,41) == 0);
  advance(1); service(); assert(messages(0xB9,21,41) == 1);
  outgoing.clear(); wm8960_midi_service(750, 35);
  assert(messages(0xB9,21,73) == 1);
  advance(299999); wm8960_midi_service(750, 35);
  assert(messages(0xB9,21,73) == 1);
  advance(1); wm8960_midi_service(750, 35);
  assert(messages(0xB9,21,73) == 2);
  for (uint32_t speed = 0; speed <= 1000; ++speed) {
    outgoing.clear(); wm8960_midi_service(750, speed);
    auto packet = std::find_if(outgoing.begin(), outgoing.end(), [](const auto &p) {
      return p[1] == 0xB9 && p[2] == 21;
    });
    assert(packet != outgoing.end() && (*packet)[3] > 0 && (*packet)[3] <= 127);
    const int decoded = static_cast<int>((*packet)[3] / 2.1333 + 1.5);
    assert(decoded == static_cast<int>(speed < 1 ? 1 : speed > 60 ? 60 : speed));
    outgoing.clear(); wm8960_midi_service(750, speed);
    assert(outgoing.empty());
  }
  fresh(); writable = false; wm8960_midi_service(750, 30);
  assert(messages(0xB9,21,62) == 0 && queue_count > 0);
  writable = true; wm8960_midi_service(750, 30);
  assert(messages(0xB9,21,62) == 1);
  fresh(); lockable = false; wm8960_midi_service(750, 40);
  assert(messages(0xB9,21,83) == 0);
  lockable = true; wm8960_midi_service(750, 40);
  assert(messages(0xB9,21,83) == 1);
  fresh(); mounted = false; service(); outgoing.clear();
  clock_ms = UINT32_MAX - 100000;
  mounted = true; service(); outgoing.clear();
  advance(299999); service(); assert(messages(0xB9,21,41) == 0);
  advance(1); service(); assert(messages(0xB9,21,41) == 1);
#ifdef DEBUG_MIDI_MESSAGES
  fresh();
  terminal.open = true; service();
  terminal.output.clear();
  message_trace_read = message_trace_count = 0;
  message_trace_dropped = 0;
  terminal.open = true; terminal.write_space = 0;
  receive(0xB9,21,35,11);
  assert(message_trace_count >= 2 && terminal.output.empty());
  terminal.write_space = 512;
  for (int count = 0; count < 10; ++count) service();
  assert(terminal.output.find("[MIDI RX]") != std::string::npos);
  assert(terminal.output.find("USB=0B B9 15 23 cable=0 cin=11 ch=10 CC data1=21 data2=35") != std::string::npos);
  assert(terminal.output.find("[MIDI TX]") != std::string::npos);
  terminal.output.clear(); message_trace_read = message_trace_count = 0;
  writable = false; wm8960_midi_wheel_step(1); service();
  assert(message_trace_count == 0);
  writable = true; service();
  assert(terminal.output.find("[MIDI TX]") != std::string::npos);
  terminal.output.clear();
  receive(0x99,17,127);
  assert(terminal.output.empty() && message_trace_count > 0);
  receive(0x99,17,0);
  for (int count = 0; count < 10; ++count) service();
  assert(terminal.output.find("NOTE_ON data1=17 data2=127") != std::string::npos);
  assert(terminal.output.find("NOTE_ON_ZERO/OFF") != std::string::npos);
  terminal.output.clear(); terminal.open = false;
  message_trace_read = message_trace_count = 0;
  const uint8_t cc[4] = {0x0B,0xB9,21,40};
  for (int count = 0; count < kMessageTraceSize + 3; ++count) {
    trace_message(cc, true);
  }
  assert(message_trace_count == kMessageTraceSize && message_trace_dropped == 3);
  service(); assert(message_trace_count == kMessageTraceSize);
  terminal.open = true;
  for (int count = 0; count <= kMessageTraceSize; ++count) print_message_trace(terminal);
  assert(message_trace_count == 0 && message_trace_dropped == 0);
  assert(terminal.output.find("dropped 3 messages") != std::string::npos);
  terminal.open = false;
#endif
  fresh();
  for (int &pin : pin_values) pin = HIGH;
  init_GPIO_key_pins();
  assert(pin_modes.back() == (std::array<int,2>{pin_straight_key, INPUT_PULLUP}));
  assert(straight_key_state == HIGH);
  pin_values[pin_straight_key] = LOW; straight_key_intr_handler();
  assert(straight_key_state == LOW);
  pin_values[pin_straight_key] = HIGH; straight_key_intr_handler();
  assert(straight_key_state == HIGH);
  pin_values[pin_straight_key] = LOW; init_GPIO_key_pins();
  assert(straight_key_state == LOW);
  pin_values[pin_straight_key] = HIGH;
  pin_modes.clear(); interrupt_pins.clear();
  configuration.pot_activated = 1;
  initialize_potentiometer();
  assert(configuration.pot_activated == 0 && pin_modes.empty());
#ifdef FEATURE_POTENTIOMETER
#error "Zero potentiometer must disable all analog speed feature code"
#endif
  initialize_rotary_encoder();
  assert(pin_modes.size() == (rotary_switch ? 3 : 2));
  assert(pin_modes[0][1] == INPUT_PULLUP && pin_modes[1][1] == INPUT_PULLUP);
  assert(interrupt_pins == std::vector<int>({rotary_pin1, rotary_pin2}));
  // Capture complete turns without any foreground polling or display servicing.
  clock_ms = 10000;
  auto pins = [](int value) {
    pin_values[rotary_pin1] = value & 1;
    pin_values[rotary_pin2] = (value >> 1) & 1;
    advance(100); encoder_irq();
  };
  auto turn = [&](bool reverse) {
    if (reverse) for (int value : {2,0,1,3}) pins(value);
    else for (int value : {1,0,2,3}) pins(value);
  };
  const int steps_per_turn =
#ifdef OPTION_ENCODER_HALF_STEP_MODE
    2;
#else
    1;
#endif
  pins(3);
  for (int n = 0; n < 20; ++n) turn(false);
  for (int n = 0; n < 20; ++n) turn(true);
  advance(1000);
  for (int n = 0; n < 20 * steps_per_turn; ++n) assert(chk_rotary_encoder() == -1);
  for (int n = 0; n < 20 * steps_per_turn; ++n) assert(chk_rotary_encoder() == 1);
  assert(chk_rotary_encoder() == 0);
  // Incomplete contact bounce must not produce a step.
  for (int value : {1,3,1,3}) pins(value);
  assert(chk_rotary_encoder() == 0);
  for (int n = 0; n < 140; ++n) turn(false);
  assert(encoder_event_overflow);
  terminal.output.clear(); chk_rotary_encoder();
  assert(!encoder_event_overflow && terminal.output.find("encoder event queue overflow") != std::string::npos);
  while (chk_rotary_encoder()) {}
  for (auto function : {EncoderFunction::Vfo, EncoderFunction::Master,
      EncoderFunction::Sidetone, EncoderFunction::Frequency,
      EncoderFunction::Speed, EncoderFunction::Microphone}) {
    encoder_function = function; draw_encoder_mode(6, false);
    assert(lcd.label == std::string(1, "VMSFWU"[static_cast<int>(function)]));
    int draws = lcd.draws;
    draw_encoder_mode(6, false); assert(lcd.draws == draws);
    draw_encoder_mode(6, true); assert(lcd.draws == draws + 1);
  }
  encoder_function = rotary_switch ? EncoderFunction::Vfo : EncoderFunction::Master;
  if (rotary_switch) {
    assert(encoder_function == EncoderFunction::Vfo);
    auto click = []() {
      pin_values[rotary_switch] = LOW; check_encoder_switch();
      advance(20); check_encoder_switch();
      advance(50); pin_values[rotary_switch] = HIGH; check_encoder_switch();
      advance(20); check_encoder_switch();
    };
    pin_values[rotary_switch] = LOW; check_encoder_switch();
    advance(10); pin_values[rotary_switch] = HIGH; check_encoder_switch();
    advance(20); check_encoder_switch();
    assert(encoder_function == EncoderFunction::Vfo);
    for (auto function : {EncoderFunction::Master, EncoderFunction::Sidetone,
        EncoderFunction::Frequency, EncoderFunction::Speed,
        EncoderFunction::Microphone, EncoderFunction::Vfo}) {
      click(); assert(encoder_function == function && !lcd_text.empty());
    }
    click(); assert(encoder_function == EncoderFunction::Master);
    pin_values[rotary_switch] = LOW; check_encoder_switch();
    advance(20); check_encoder_switch();
    advance(999); check_encoder_switch();
    assert(encoder_function == EncoderFunction::Master);
    advance(1); check_encoder_switch();
    assert(encoder_function == EncoderFunction::Vfo);
    pin_values[rotary_switch] = HIGH; check_encoder_switch();
    advance(20); check_encoder_switch();
    assert(encoder_function == EncoderFunction::Vfo);
    click();
    clock_ms = UINT32_MAX-500;
    pin_values[rotary_switch] = LOW; check_encoder_switch();
    advance(20); check_encoder_switch(); advance(1000); check_encoder_switch();
    assert(encoder_function == EncoderFunction::Vfo);
  } else {
    assert(encoder_function == EncoderFunction::Master);
    check_encoder_switch(); assert(encoder_function == EncoderFunction::Master);
  }
  encoder_function = EncoderFunction::Master;
  for (int level = 0; level <= 100; ++level) {
    assert(step_audio_volume(level, true) ==
        std::min(100, std::max(level + 1, (level * 1122 + 500) / 1000)));
    assert(step_audio_volume(level, false) ==
        std::max(0, std::min(level - 1, (level * 891 + 500) / 1000)));
  }
  configuration.usb_play_volume = 100; configuration.usb_play_muted = true;
  lcd_text = "unchanged";
  apply_encoder_step(-1);
  assert(configuration.usb_play_volume == 89 && !configuration.usb_play_muted);
  assert(lcd_text == "unchanged");
  encoder_function = EncoderFunction::Sidetone;
  configuration.sidetone_volume = 50; apply_encoder_step(1);
  assert(configuration.sidetone_volume == 56);
  encoder_function = EncoderFunction::Microphone;
  configuration.usb_mic_volume = 0; apply_encoder_step(-1);
  assert(configuration.usb_mic_volume == 0);
  apply_encoder_step(1); assert(configuration.usb_mic_volume == 1);
  encoder_function = EncoderFunction::Frequency;
  configuration.hz_sidetone = 750; apply_encoder_step(1);
  assert(configuration.hz_sidetone == 760);
  configuration.hz_sidetone = 300; apply_encoder_step(-1);
  assert(configuration.hz_sidetone == 300);
  configuration.hz_sidetone = 1200; apply_encoder_step(1);
  assert(configuration.hz_sidetone == 1200);
  encoder_function = EncoderFunction::Speed;
  configuration.wpm = 20; apply_encoder_step(2); assert(configuration.wpm == 22);
  configuration.wpm_command_mode = 15; keyer_machine_mode = KEYER_COMMAND_MODE;
  apply_encoder_step(-1);
  assert(configuration.wpm_command_mode == 14 && configuration.wpm == 22);
  keyer_machine_mode = 0; encoder_function = EncoderFunction::Vfo;
  lcd_text = "unchanged";
  show_encoder_function(); assert(lcd_text == "unchanged");
  outgoing.clear(); apply_encoder_step(1); service();
  assert(lcd_text == "unchanged");
  assert(messages(0xB9,20,ENCODER_VFO_DIRECTION < 0 ? 1 : 127) == 1);
  apply_encoder_step(-1); service();
  assert(messages(0xB9,20,127) == 1 && messages(0xB9,20,1) == 1);
  // A backlog must send in bounded batches without waiting for clock ticks.
  configuration.hz_sidetone = 750;
  outgoing.clear(); lcd_text = "unchanged";
  for (int n = 0; n < 20; ++n) turn(false);
  const uint32_t before_send = clock_ms;
  check_rotary_encoder();
  assert(messages(0xB9,20,ENCODER_VFO_DIRECTION < 0 ? 127 : 1) == 16);
  assert(clock_ms == before_send && lcd_text == "unchanged");
  check_rotary_encoder();
  if (steps_per_turn == 2) check_rotary_encoder();
  assert(messages(0xB9,20,ENCODER_VFO_DIRECTION < 0 ? 127 : 1) == 20 * steps_per_turn);
  assert(chk_rotary_encoder() == 0 && clock_ms == before_send && lcd_text == "unchanged");
  for (auto function : {EncoderFunction::Master, EncoderFunction::Sidetone,
      EncoderFunction::Frequency, EncoderFunction::Speed, EncoderFunction::Microphone}) {
    encoder_function = function;
    configuration.wpm = 100; configuration.hz_sidetone = 1000;
    configuration.usb_play_volume = configuration.sidetone_volume = configuration.usb_mic_volume = 100;
    lcd_text = "unchanged";
    for (int n = 0; n < 20; ++n) turn(false);
    const uint32_t start = clock_ms;
    check_rotary_encoder();
    assert(((encoder_event_head - encoder_event_tail) & 127) == 20 * steps_per_turn - 16);
    assert(lcd_text == "unchanged" && clock_ms == start);
    check_rotary_encoder();
    if (steps_per_turn == 2) check_rotary_encoder();
    assert(encoder_event_head == encoder_event_tail);
    if (function == EncoderFunction::Speed) assert(configuration.wpm == 100 - 20 * steps_per_turn);
    if (function == EncoderFunction::Frequency) assert(configuration.hz_sidetone == 1000 - 200 * steps_per_turn);
    advance(99); check_rotary_encoder(); assert(lcd_text == "unchanged");
    advance(1); check_rotary_encoder(); assert(lcd_text != "unchanged");
    lcd_text = "no repeat"; advance(100); check_rotary_encoder();
    assert(lcd_text == "no repeat");
  }
  encoder_function = EncoderFunction::Vfo;
  mounted = false; service(); apply_encoder_step(1);
  assert(lcd_text == "MIDI unavailable");
  puts("PASS: MIDI/audio, encoder IRQ capture, bounce, overflow, mode status, menu, VFO direction, potentiometer disabled");
}
'@

$temp = Join-Path ([IO.Path]::GetTempPath()) ("keyer-midi-test-" + [guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $temp
$source = Join-Path $temp 'test.cpp'
$exe = Join-Path $temp 'test.exe'
$obj = Join-Path $temp 'test.obj'
try {
    Set-Content -LiteralPath $source -Value (
        $mock + "`n" + $sine + "`n" + ($audioFunctions -join "`n") +
        "`n" + $header + "`n" + $midi + "`n" + $config +
        "`n" + $cli + "`n" + $potGuard + "`n" + $encoderMocks +
        "`n" + $keyInterrupts + "`n" + $encoderDirection + "`n" + $encoderTable + "`n" + $encoderCapture +
        "`n" + $encoder + "`n" + ($encoderFunctions -join "`n") + "`n" + $checks
    ) -Encoding ASCII
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ tools required' }
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual Studio C++ tools not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    foreach ($defines in @('', '/DDEBUG_MIDI', '/DDEBUG_MIDI_MESSAGES', '/DDEBUG_MIDI /DDEBUG_MIDI_MESSAGES', '/Drotary_switch=0', '/Drotary_switch=42', '/DENCODER_VFO_DIRECTION=1', '/DTEST_ENCODER_FULL_STEP')) {
        & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W3 $defines /Fo`"$obj`" /Fe`"$exe`" `"$source`""
        if ($LASTEXITCODE -ne 0) { throw "Host compilation failed: $defines" }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw "Host assertions failed: $defines" }
    }
} finally {
    foreach ($file in @($source, $exe, $obj)) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    Remove-Item -LiteralPath $temp
}
