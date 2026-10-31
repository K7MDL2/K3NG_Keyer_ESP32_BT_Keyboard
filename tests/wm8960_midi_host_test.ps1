$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$midi = Get-Content (Join-Path $root 'main\wm8960_midi.cpp') -Raw
$header = Get-Content (Join-Path $root 'main\wm8960_midi.h') -Raw
$audio = Get-Content (Join-Path $root 'main\wm8960_audio.cpp') -Raw
$main = Get-Content (Join-Path $root 'main\main.cpp') -Raw
$midi = [regex]::Replace($midi, '(?m)^#include .*\r?$', '')
$header = [regex]::Replace($header, '(?m)^#include .*\r?$', '')

function Get-CppFunction([string]$text, [string]$name) {
    $match = [regex]::Match($text, "(?m)^(?:bool|void|int16_t|uint16_t) $name\([^;]*?\) \{")
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
constexpr uint8_t grid_len_max = 9;
class String {
  std::string text;
 public:
  String(const char *value = "") : text(value) {}
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

$checks = @'
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
#ifdef SEND_MIDI_FREQUENCY
  outgoing.clear(); wm8960_midi_service(300, 20);
  assert(messages(0xB9,3,0) == 1);
  outgoing.clear(); wm8960_midi_service(1000, 20);
  assert(messages(0xB9,3,127) == 1);
  outgoing.clear(); wm8960_midi_service(1500, 20);
  assert(messages(0xB9,3,127) == 0);
  advance(1000); wm8960_midi_service(1500, 20);
  assert(messages(0xB9,3,127) == 1);
#else
  assert(messages(0xB9,3,0) == 0 && messages(0xB9,3,127) == 0);
#endif
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
    wm8960_midi_service(750, speed);
    assert(outgoing.size() <= 2); // Optional pitch may also be reported.
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
  puts("PASS: MIDI packets, PTT modes, input sources, echo, DMA render, queue recovery, disconnect, rollover, CLI, EEPROM, optional pitch");
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
        "`n" + $cli + "`n" + $checks
    ) -Encoding ASCII
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ tools required' }
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual Studio C++ tools not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    foreach ($defines in @('', '/DDEBUG_MIDI', '/DSEND_MIDI_FREQUENCY', '/DDEBUG_MIDI /DSEND_MIDI_FREQUENCY', '/DDEBUG_MIDI_MESSAGES', '/DDEBUG_MIDI /DDEBUG_MIDI_MESSAGES /DSEND_MIDI_FREQUENCY')) {
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
