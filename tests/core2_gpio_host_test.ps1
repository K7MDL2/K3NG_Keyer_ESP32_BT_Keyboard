$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$features = Get-Content (Join-Path $root 'main\keyer_features_and_options_esp32_dev.h') -Raw
$pins = Get-Content (Join-Path $root 'main\keyer_pin_settings_esp32_dev.h') -Raw
$main = Get-Content (Join-Path $root 'main\main.cpp') -Raw
$settings = Get-Content (Join-Path $root 'main\keyer_settings_esp32_dev.h') -Raw
$buttonStart = $main.IndexOf('enum button_ID{')
$buttonEnd = $main.IndexOf('void process_buttons();', $buttonStart)
$buttons = $main.Substring($buttonStart, $buttonEnd - $buttonStart)
$assignment = [regex]::Match($settings, '(?s)#define BUTTON_ASSIGNMENT_ORDER \{ BUTTON_PAUSE.*?\}').Value
$layout = [regex]::Match($main, '(?s)#if defined\(FEATURE_M5STACK_CORE2\)\r?\n\s*#define BUTTON_FONT.*?(?=\s*#elif)').Value + "`n#endif"
$cwLayout = [regex]::Match($main, '(?s)#ifdef FEATURE_M5STACK_CORE2\r?\n\s*#undef TFT_FONT_MEDIUM.*?\r?\n\s*#endif').Value
$rows = [regex]::Match($settings, '(?s)#ifdef FEATURE_M5STACK_CORE2\r?\n\s*#define LCD_COLUMNS.*?(?=#elif)').Value + "`n#endif"
foreach ($datum in @(@('BL_DATUM', 8), @('BC_DATUM', 9), @('MC_DATUM', 5))) {
    if ($main -notmatch "#define $($datum[0]) $($datum[1])\b") { throw "Invalid Core2 $($datum[0])" }
}
if (!$cwLayout -or !$rows) { throw 'Core2 CW layout missing' }
$buttonInit = [regex]::Match($main, '(?s)void initialize_buttons\(\) \{.*?\r?\n\}').Value
$touchRead = [regex]::Match($main, '(?s)bool read_core2_touch\(.*?\r?\n\}').Value
$tones = [regex]::Match($main, '(?s)void s_tone\(uint8_t sidetone_line_pin_num, uint16_t frequency, uint32_t duration_ms\).*?(?=\r?\n\r?\nvoid debug_blink)').Value
$audio = "#ifdef FEATURE_CORE2_AUDIO`n" + [regex]::Match($main, '(?s)int audio_volume\(AudioVolumeTarget target\) \{.*?(?=\r?\n#if defined\(FEATURE_WM8960_USB_AUDIO\).*?\r?\n#if defined\(FEATURE_SERIAL\).*?\r?\nbool cli_mic)').Value
if (!$audio) { throw 'Shared audio volume code not found' }
$buttonProcess = [regex]::Match($main, '(?s)void process_buttons\(\) \{.*?\r?\n\}').Value
$inputs = [regex]::Match($main, '(?s)// ESP32 local pin interrupt setup.*?(?=//-{10,}\s*#if defined\(FEATURE_MCP23017_EXPANDER\))').Value
if (!$assignment -or !$buttonInit -or !$touchRead -or !$buttonProcess) { throw 'Core2 button code not found' }
$boardInit = [regex]::Match($main, '(?s)void initialize_core2_board\(\) \{.*?\r?\n\}(?=\r?\n#endif)').Value
$portAMode = [regex]::Match($main, '(?s)void load_core2_port_a_mode\(\) \{.*?\r?\n\}').Value + "`n" +
    [regex]::Match($main, '(?s)bool cli_core2_port_a\(PRIMARY_SERIAL_CLS \*port, String command\) \{.*?\r?\n\}').Value
$paddleRead = [regex]::Match($main, '(?s)int paddle_pin_read\(int pin_to_read\) \{.*?\r?\n\}').Value
$beaconToggle = [regex]::Match($main, '(?s)\tvoid toggle_beacon_setting\(\) \{.*?\r?\n\t\}').Value
if ($main -notmatch '(?s)case PS2_Q_CTRL:\s*toggle_beacon_setting\(\);' -or
    $main -notmatch '(?s)case ''_'':.*?toggle_beacon_setting\(\);') { throw 'Beacon keyboard/CLI routing missing' }
$outputInit = [regex]::Match($main, '(?s)\tif \(tx_key_line_1\) \{\r?\n\t\tkeyer_output_initialize.*?(?=\tif \(sidetone_line\))').Value
$outputHelpers = [regex]::Match($main, '(?s)void keyer_output_write\(uint8_t pin, uint8_t state\) \{.*?(?=\r?\nvoid ptt_key)').Value
if (!$boardInit) { throw 'Core2 board initialization not found' }
$sourceText = @'
#include <array>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <string>
#include <algorithm>
#include <atomic>
#include <cstdarg>
#include "consumer_audio.hpp"
#define PROJECT_ESP32_COMPILER
#define HARDWARE_ESP32_DEV
#define DISPLAY_TYPE M5STACK_CORE2_LCD
'@
$mocks = @'
namespace lgfx {
struct LGFX_Button {
  bool pressed = false;
  void press(bool value) { pressed = value; }
  bool isPressed() { return pressed; }
};
}
#define IRAM_ATTR
using gpio_num_t = int;
constexpr int GPIO_MODE_INPUT = 1, GPIO_PULLUP_ONLY = 2, GPIO_INTR_ANYEDGE = 3;
int levels[40] = {}, directions[40] = {}, pulls[40] = {};
bool paddle_left_state = false, paddle_right_state = false, straight_key_state = false;
int gpio_install_isr_service(int) { return 0; }
int gpio_set_direction(int pin, int mode) { directions[pin] = mode; return 0; }
int gpio_set_pull_mode(int pin, int mode) { pulls[pin] = mode; return 0; }
int gpio_set_intr_type(int, int) { return 0; }
int gpio_isr_handler_add(int, void (*)(void *), void *) { return 0; }
int gpio_get_level(int pin) { return levels[pin]; }
constexpr int HIGH = 1, LOW = 0, OUTPUT = 4, GPIO_MODE_OUTPUT_OD = 5, GPIO_FLOATING = 6;
int digitalRead(uint8_t pin) { return levels[pin]; }
constexpr int tx_key_line_inactive_state = 0, ptt_line_inactive_state = 0;
constexpr int tx_key_line_active_state = 1, ptt_line_active_state = 1;
bool arduino_gpio[40] = {};
void digitalWrite(uint8_t pin, uint8_t state) { assert(arduino_gpio[pin]); levels[pin]=state; }
void pinMode(uint8_t pin, int mode) { arduino_gpio[pin]=true; directions[pin]=mode; }
int gpio_set_level(gpio_num_t pin, int state) { levels[pin]=state; return 0; }
namespace m5 { enum class board_t { board_M5StackCore2, board_unknown }; }
struct SpeakerConfig {
  uint32_t sample_rate = 48000;
  size_t dma_buf_len = 256, dma_buf_count = 8;
  uint8_t task_priority = 2;
};
struct SpeakerMock {
  SpeakerConfig cfg;
  bool begin_ok = true, tone_ok = true;
  int begins = 0, stops = 0, tones = 0, channel = -1, volume = 0;
  uint16_t frequency = 0;
  uint32_t duration = 0;
  SpeakerConfig config() { return cfg; }
  void config(SpeakerConfig value) { cfg = value; }
  void setVolume(int value) { volume = value; }
  bool begin() { ++begins; return begin_ok; }
  bool tone(uint16_t hz, uint32_t ms, int ch, bool replace) {
    assert(replace); ++tones; frequency = hz; duration = ms; channel = ch; return tone_ok;
  }
  void stop(int ch) { ++stops; channel = ch; }
};
struct BoardConfig {
  m5::board_t fallback_board = m5::board_t::board_unknown;
  bool internal_spk = true, internal_mic = true, internal_imu = true, internal_rtc = true;
  int external_display_value = 255;
  bool output_power = true;
};
struct Board {
  SpeakerMock Speaker;
  BoardConfig applied;
  m5::board_t board = m5::board_t::board_M5StackCore2;
  BoardConfig config() { return {}; }
  void begin(BoardConfig cfg) { applied = cfg; }
  m5::board_t getBoard() { return board; }
} M5;
struct Display {
  bool has_touch = true;
  bool pressed = false;
  uint16_t raw_x = 0, raw_y = 0;
  int w = 320, h = 240, rotation = 0, brightness = 0;
  bool touch() { return has_touch; }
  void setRotation(int value) { rotation = value; }
  void setBrightness(int value) { brightness = value; }
  int width() { return w; }
  int height() { return h; }
  bool getTouchRaw(uint16_t *x, uint16_t *y) { *x = raw_x; *y = raw_y; return pressed; }
  void convertRawXY(uint16_t *, uint16_t *) {}
} lcd;
struct Log {
  int errors = 0;
  std::string output;
  void println(const char *text) { output += text; output += '\n'; if (std::string(text).find("Error:") == 0) ++errors; }
  void println() { output += '\n'; }
  void printf(const char *fmt, ...) { char buffer[256]; va_list args; va_start(args,fmt); vsnprintf(buffer,sizeof(buffer),fmt,args); va_end(args); output += buffer; }
} log_port;
Log *debug_serial_port = &log_port;
uint32_t delayed_ms = 0;
void myDelay(uint32_t ms) { delayed_ms += ms; }
#define F(x) x
#define ESP_FAIL 1
#define ESP_ERROR_CHECK(x) do { if (x) throw std::runtime_error("board init failed"); } while (0)
using PRIMARY_SERIAL_CLS = Log;
struct String : std::string {
  using std::string::string;
  String(const std::string &value) : std::string(value) {}
  String(int value) : std::string(std::to_string(value)) {}
  void trim() { const auto start=find_first_not_of(" \t\r\n"); if(start==npos) clear(); else *this=substr(start,find_last_not_of(" \t\r\n")-start+1); }
  int indexOf(char c) const { auto index=find(c); return index==npos ? -1 : int(index); }
  String substring(size_t start) const { return start<size() ? substr(start) : ""; }
  String substring(size_t start,size_t end) const { return substr(start,end-start); }
  size_t length() const { return size(); }
  long toInt() const { return std::stol(*this); }
};
int constrain(int value, int low, int high) { return std::clamp(value,low,high); }
enum class AudioVolumeTarget : uint8_t { Playback, Sidetone, Microphone, LocalPlayback };
struct Config { int sidetone_volume=100, usb_play_volume=100, usb_mic_volume=100, current_ptt_line=26; bool sidetone_muted=false,usb_play_muted=false,usb_mic_muted=false,beacon_mode_on_boot_up=false; } configuration;
int config_dirty = 0;
bool core2_port_a_output = false, core2_port_a_saved_output = false;
bool core2_port_a_reversed = false, core2_port_a_saved_reversed = false;
bool core2_port_a_key_active = false, core2_port_a_ptt_active = false;
int current_tx_key_line = 25;
#define CORE2_PORT_A_OUTPUT_DEFAULT 0
#define CORE2_PORT_A_REVERSED_DEFAULT 0
struct Preferences {
  static inline int mode = -1;
  static inline int polarity = -1;
  static inline bool open_ok = true, write_ok = true;
  bool begin(const char *, bool) { return open_ok; }
  uint8_t getUChar(const char *key, uint8_t fallback) { int value=std::string(key)=="porta" ? mode : polarity; return value < 0 ? fallback : value; }
  size_t putUChar(const char *key, uint8_t value) { if (!write_ok) return 0; (std::string(key)=="porta" ? mode : polarity)=value; return 1; }
  void end() {}
};
bool popup_active = false, ptt_line_activated = false, key_state = false;
int in_pairing_mode = 0;
String audio_message;
unsigned int audio_message_duration = 0;
int audio_messages = 0;
constexpr unsigned int default_display_msg_delay = 1000;
void lcd_center_print_timed(String text, uint8_t row, unsigned int duration) {
  assert(row == 0);
  audio_message = text;
  audio_message_duration = duration;
  ++audio_messages;
}
#define FEATURE_SERIAL
#define FEATURE_COMMAND_LINE_INTERFACE
#define FEATURE_DISPLAY
'@
$buttonMocks = @'
int created = 0, refreshed = -1, queued = -1, action = -1;
unsigned long last_active_time = 0;
const int number_of_memories = 8;
const char *btn2_text = "", *btn3_text = "";
enum { PS2_UPARROW, PS2_DOWNARROW, PS2_F_CTRL, PS2_T_CTRL, PS2_L_CTRL, PS2_O_CTRL, PS2_ESC };
unsigned long millis() { return 100; }
void create_buttons() { ++created; }
void refresh_button_row(uint8_t row) { refreshed = row; }
void queueadd(int code) { queued = code; }
void queueflush() {}
void pause_btn_toggle_key(uint16_t) { action = BUTTON_PAUSE; }
void list_memory_key(uint16_t, uint8_t) { action = BUTTON_MEM_LIST; }
void memX_key(uint16_t, uint8_t memory) { action = BUTTON_MEM_1 + memory - 1; }
void TX_select_key(uint16_t) { action = BUTTON_TX_SELECT; }
void TX_enable_key(uint16_t) { action = BUTTON_TX_ENABLE; }
void generic_popup_key(uint16_t, const char *) {}
void clear_holds_key() {}
void popup(bool) {}
'@
$buttonChecks = @'
  static_assert(BUTTONS_PER_ROW == 3 && buttonCount == 4 && MAX_ROWS == 8 && NUM_KEYS == 25);
  static_assert(LCD_ROWS == 3 && LCD_COLUMNS == 12);
  static_assert(SCROLL_TEXT_LEFT_SIDE + LCD_COLUMNS * COLUMN_WIDTH <= 315);
  static_assert(SCROLL_TEXT_TOP_LINE == 67);
  static_assert(SCROLL_TEXT_TOP_LINE - 29 >= 29);
  static_assert(SCROLL_TEXT_TOP_LINE + (LCD_ROWS - 1) * FONT_HEIGHT < 167);
  initialize_buttons();
  assert(created == 1);
  for (int row = 0; row < MAX_ROWS; ++row) {
    int counts[3] = {};
    for (const auto &entry : key) {
      if (entry.row != row) continue;
      assert(entry.btn_idx < 3);
      ++counts[entry.btn_idx];
      if (!entry.btn_idx) {
        char expected[7];
        snprintf(expected, sizeof(expected), "%d/%d", row + 1, NUM_BUTTON_ROWS);
        assert(!strcmp(entry.text_off, expected));
      }
    }
    assert(counts[0] == 1 && counts[1] == 1 && counts[2] == 1);
  }
  for (int i = 0; i < b_array_size; ++i) {
    int matches = 0;
    for (const auto &entry : key) if (entry.key_event == b_array[i]) {
      assert(entry.row == i / 2 && entry.btn_idx == i % 2 + 1);
      ++matches;
    }
    assert(matches == 1);
  }
  for (int row = 0; row < NUM_BUTTON_ROWS; ++row) {
    assert(button_row == row);
    btn[0].len = 1;
    btn[0].p_btn.press(true);
    process_buttons();
    assert(button_row == (row + 1) % NUM_BUTTON_ROWS);
    assert(refreshed == button_row && last_active_time == 100);
  }
  for (int column = 1; column <= 2; ++column) {
    btn[column].len = 1;
    btn[column].p_btn.press(true);
    process_buttons();
    assert(action == (column == 1 ? BUTTON_PAUSE : BUTTON_TX_ENABLE));
  }
  btn[2].len = 3;
  btn[2].p_btn.press(true);
  process_buttons();
  assert(queued == PS2_F_CTRL);
  action = -1;
  btn[1].len = 2;
  btn[1].p_btn.press(true);
  process_buttons();
  assert(action == -1);
  btn[1].p_btn.press(false);
  btn[3].len = 1;
  btn[3].p_btn.press(true);
  process_buttons();
  assert(queued == PS2_ESC);
  process_buttons(); // No pending button must not index beyond the key array.
  uint16_t x = 0, y = 0;
  assert(!read_core2_touch(&x, &y));
  lcd.pressed = true;
  lcd.raw_y = 260;
  for (int raw_x : {0, 106, 107, 213, 214, 319}) {
    lcd.raw_x = raw_x;
    assert(read_core2_touch(&x, &y));
    assert(x == 1 + (raw_x * 3 / 320) * (BUTTON_WIDTH + 4) + BUTTON_WIDTH / 2);
    assert(y == BUTTON_ROW + BUTTON_HEIGHT / 2);
  }
  lcd.raw_x = 25;
  lcd.raw_y = 239;
  assert(read_core2_touch(&x, &y) && x == 25 && y == 239);
  puts("PASS: Core2 4/8-page mapping, page wrap, actions, hold/abort and physical A/B/C strip");
'@
$checks = @'
#ifndef FEATURE_M5STACK_CORE2
#error Core2 profile was not selected
#endif
#if defined(FEATURE_GPS) || defined(FEATURE_COMPASS) || defined(FEATURE_MCP23017_EXPANDER)
#error Core2 initial bring-up must use direct GPIO without GPS/compass
#endif
#if !defined(FEATURE_TFT_DISPLAY) || !defined(FEATURE_TOUCH_DISPLAY) || !defined(FEATURE_STRAIGHT_KEY)
#error Core2 display, touch and straight key must be enabled
#endif
int main() {
  const uint8_t bitmap_map[] = {0x05,0x0C,0x85,3,0x09,0xE2,0x09,0xE9,0x09,0xEA,0x75,1,0x95,3,0x81,2,0x75,5,0x95,1,0x81,1};
  const uint8_t array_map[] = {0x05,0x0C,0x85,4,0x19,0,0x2A,0xFF,0x03,0x75,16,0x95,1,0x81,0};
  uint8_t mask = 0;
  for (uint8_t bits = 0; bits < 8; ++bits) {
    assert(decode_consumer_audio(bitmap_map,sizeof(bitmap_map),3,&bits,1,mask));
    assert(mask == bits);
  }
  for (uint16_t usage : {0xE2,0xE9,0xEA,0}) {
    uint8_t data[] = {uint8_t(usage),uint8_t(usage>>8)};
    assert(decode_consumer_audio(array_map,sizeof(array_map),4,data,2,mask));
    assert(mask == (usage==0xE2 ? 1 : usage==0xE9 ? 2 : usage==0xEA ? 4 : 0));
  }
  uint8_t sample = 1;
  assert(!decode_consumer_audio(bitmap_map,sizeof(bitmap_map),2,&sample,1,mask));
  assert(!decode_consumer_audio(bitmap_map,sizeof(bitmap_map)-1,3,&sample,1,mask));
  assert(!decode_consumer_audio(array_map,sizeof(array_map),4,&sample,1,mask));
  #ifdef TEST_DEFAULT_ROWS
  static_assert(BUTTON_ROWS == 4);
  #endif
  static_assert(MAX_TX_PORTS == 2);
  for (int level : {0, 1}) {
    levels[pin_straight_key] = level;
    init_ESP32_GPIO_key_pins();
    assert(directions[pin_straight_key] == GPIO_MODE_INPUT && pulls[pin_straight_key] == GPIO_PULLUP_ONLY);
    assert(directions[35] == 0 && pulls[35] == 0);
    assert(straight_key_state == bool(level));
    assert(pulls[paddle_left] == GPIO_PULLUP_ONLY && pulls[paddle_right] == GPIO_PULLUP_ONLY);
  }
  const std::array<int, 7> pins = {paddle_left, paddle_right, pin_straight_key,
      tx_key_line_1, ptt_tx_1, tx_key_line_2, ptt_tx_2};
  const std::array<int, 18> reserved = {0, 1, 2, 3, 4, 5, 12, 15, 16, 17, 18, 21, 22, 23, 34, 35, 38, 39};
  for (size_t i = 0; i < pins.size(); ++i) {
    for (size_t j = i + 1; j < pins.size(); ++j) assert(pins[i] != pins[j]);
    for (int pin : reserved) assert(pins[i] != pin);
  }
  for (int pin : {tx_key_line_1, ptt_tx_1, tx_key_line_2, ptt_tx_2}) assert(pin > 0 && pin < 34);
  assert(paddle_left == 33 && paddle_right == 32 && pin_straight_key == 14);
  assert(tx_key_line_1 == 25 && ptt_tx_1 == 26 && tx_key_line_2 == 27 && ptt_tx_2 == 19);
  load_core2_port_a_mode();
  assert(!core2_port_a_output && current_tx_key_line == 25 && configuration.current_ptt_line == 26);
  assert(cli_core2_port_a(&log_port, "PORTA OUTPUT"));
  assert(!core2_port_a_output && core2_port_a_saved_output && Preferences::mode == 1);
  assert(log_port.output.find("reboot to apply") != std::string::npos);
  load_core2_port_a_mode();
  assert(core2_port_a_output && current_tx_key_line == 33 && configuration.current_ptt_line == 32);
  std::fill(std::begin(directions), std::end(directions), 0);
  std::fill(std::begin(pulls), std::end(pulls), 0);
  init_ESP32_GPIO_key_pins();
  assert(directions[32] == 0 && directions[33] == 0 && directions[25] == 0 && directions[26] == 0);
  assert(pulls[32] == 0 && pulls[33] == 0 && paddle_left_state && paddle_right_state);
  assert(directions[14] == GPIO_MODE_INPUT && pulls[14] == GPIO_PULLUP_ONLY);
  assert(paddle_pin_read(paddle_left) == HIGH && paddle_pin_read(paddle_right) == HIGH);
  initialize_test_outputs();
  assert(directions[33] == GPIO_MODE_OUTPUT_OD && directions[32] == GPIO_MODE_OUTPUT_OD);
  assert(levels[33] == HIGH && levels[32] == HIGH);
  for (int pin : {33,32}) {
    keyer_output_write(pin,HIGH); assert(levels[pin] == LOW);
    keyer_output_write(pin,LOW); assert(levels[pin] == HIGH);
  }
  keyer_output_write(27,HIGH); assert(levels[27] == HIGH);
  assert(cli_core2_port_a(&log_port, "PORTA REVERSE") && core2_port_a_saved_reversed && core2_port_a_reversed);
  assert(levels[33] == LOW && levels[32] == LOW);
  load_core2_port_a_mode();
  initialize_test_outputs();
  assert(levels[33] == LOW && levels[32] == LOW);
  for (int pin : {33,32}) {
    keyer_output_write(pin,HIGH); assert(levels[pin] == HIGH);
    keyer_output_write(pin,LOW); assert(levels[pin] == LOW);
  }
  assert(cli_core2_port_a(&log_port, "PORTA REVERSE") && !core2_port_a_saved_reversed);
  assert(levels[33] == HIGH && levels[32] == HIGH);
  keyer_output_write(33,HIGH);
  ptt_line_activated = true;
  key_state = true;
  assert(cli_core2_port_a(&log_port, "PORTA REVERSE"));
  assert(levels[33] == HIGH && levels[32] == LOW && core2_port_a_key_active && !core2_port_a_ptt_active);
  keyer_output_write(32,HIGH);
  assert(cli_core2_port_a(&log_port, "PORTA REVERSE"));
  assert(levels[33] == LOW && levels[32] == LOW && core2_port_a_key_active && core2_port_a_ptt_active);
  Preferences::write_ok = false;
  assert(cli_core2_port_a(&log_port, "PORTA REVERSE") && !core2_port_a_reversed);
  assert(levels[33] == LOW && levels[32] == LOW);
  Preferences::write_ok = true;
  keyer_output_write(33,LOW);
  keyer_output_write(32,LOW);
  assert(levels[33] == HIGH && levels[32] == HIGH);
  ptt_line_activated = false;
  key_state = false;
  load_core2_port_a_mode();
  assert(directions[25] == 0 && directions[26] == 0);
  assert(cli_core2_port_a(&log_port, "PORTA BAD") && Preferences::mode == 1);
  Preferences::write_ok = false;
  assert(cli_core2_port_a(&log_port, "PORTA PADDLES") && core2_port_a_saved_output);
  Preferences::write_ok = true;
  Preferences::open_ok = false;
  assert(cli_core2_port_a(&log_port, "PORTA PADDLES") && core2_port_a_saved_output);
  Preferences::open_ok = true;
  key_state = true;
  assert(cli_core2_port_a(&log_port, "PORTA PADDLES") && Preferences::mode == 1);
  key_state = false;
  assert(cli_core2_port_a(&log_port, "PORTA PADDLES") && core2_port_a_output && !core2_port_a_saved_output);
  load_core2_port_a_mode();
  assert(!core2_port_a_output && current_tx_key_line == 25 && configuration.current_ptt_line == 26);
  assert(!cli_core2_port_a(&log_port, "OTHER"));
  Preferences::mode = 2;
  bool invalid_mode = false;
  try { load_core2_port_a_mode(); } catch (const std::runtime_error &) { invalid_mode = true; }
  assert(invalid_mode);
  Preferences::mode = 0;
  log_port.errors = 0;
  toggle_beacon_setting();
  assert(configuration.beacon_mode_on_boot_up && config_dirty && audio_message == "Beacon On");
  config_dirty = 0;
  toggle_beacon_setting();
  assert(!configuration.beacon_mode_on_boot_up && config_dirty && audio_message == "Beacon Off");
  audio_messages = 0;
  assert(ptt_input_pin == 0);
  assert(sidetone_line == 0 && audio_enable == 0 && GPS_RX_PIN == 0);
  assert(tx_key_line_3 == 0 && tx_key_line_4 == 0 && tx_key_line_5 == 0 && tx_key_line_6 == 0);
  assert(ptt_tx_3 == 0 && ptt_tx_4 == 0 && ptt_tx_5 == 0 && ptt_tx_6 == 0);
  initialize_core2_board();
  #ifdef FEATURE_CORE2_AUDIO
  assert(M5.applied.internal_spk && M5.Speaker.begins == 1);
  assert(M5.Speaker.cfg.sample_rate == 48000 && M5.Speaker.cfg.dma_buf_len == 64);
  assert(M5.Speaker.cfg.dma_buf_count == 4 && M5.Speaker.cfg.task_priority == 3);
  assert(M5.Speaker.volume == CORE2_SPEAKER_VOLUME);
  assert(cli_audio_volume(&log_port, "SIDETONE 50"));
  assert(configuration.sidetone_volume == 50 && M5.Speaker.volume == CORE2_SPEAKER_VOLUME/2 && config_dirty);
  assert(cli_audio_volume(&log_port, "SIDETONE MUTE") && M5.Speaker.volume == 0);
  audio_consumer_key(0xE9, 0);
  assert(configuration.sidetone_volume == 54 && !configuration.sidetone_muted);
  assert(audio_messages == 0);
  service_core2_audio_message();
  assert(audio_message == "Sidetone 54%" && audio_message_duration == 1000);
  audio_consumer_key(0xE2, 0);
  assert(configuration.sidetone_muted && M5.Speaker.volume == 0);
  service_core2_audio_message();
  assert(audio_message == "Sidetone muted");
  audio_consumer_key(0xE2, 0);
  assert(!configuration.sidetone_muted);
  service_core2_audio_message();
  assert(audio_message == "Sidetone 54%");
  audio_consumer_key(0xEA, 0);
  assert(configuration.sidetone_volume == 50);
  service_core2_audio_message();
  assert(audio_message == "Sidetone 50%");
  const int previous_audio_messages = audio_messages;
  audio_consumer_key(0xE9, 0x11);
  audio_consumer_key(0x01, 0);
  service_core2_audio_message();
  assert(audio_messages == previous_audio_messages && configuration.sidetone_volume == 50);
  assert(cli_audio_volume(&log_port, "SIDETONE 101") && configuration.sidetone_volume == 50);
  assert(cli_audio_volume(&log_port, "USBPLAY 10") && configuration.sidetone_volume == 50);
  assert(log_port.errors == 2);
  log_port.errors = 0;
  assert(cli_audio_volume(&log_port, "AUDIO"));
  assert(log_port.output.find("SIDETONE 50% ON") != std::string::npos);
  assert(!cli_audio_volume(&log_port, "OTHER"));
  assert(cli_audio_volume(&log_port, "SIDETONE +") && configuration.sidetone_volume == 52);
  assert(cli_audio_volume(&log_port, "SIDETONE -") && configuration.sidetone_volume == 50);
  assert(audio_messages == previous_audio_messages);
  for (int press = 0; press < 4; ++press) audio_consumer_key(0xE9, 0);
  assert(audio_messages == previous_audio_messages && configuration.sidetone_volume == 66);
  popup_active = true;
  service_core2_audio_message();
  assert(audio_messages == previous_audio_messages);
  popup_active = false;
  service_core2_audio_message();
  assert(audio_messages == previous_audio_messages + 1 && audio_message == "Sidetone 66%");
  service_core2_audio_message();
  assert(audio_messages == previous_audio_messages + 1);
  for (int press = 0; press < 3; ++press) audio_consumer_key(0xE2, 0);
  service_core2_audio_message();
  assert(audio_message == "Sidetone muted");
  set_audio_volume(AudioVolumeTarget::Sidetone, 98, true);
  audio_consumer_key(0xE9, 0);
  assert(configuration.sidetone_volume == 100 && !configuration.sidetone_muted);
  set_audio_volume(AudioVolumeTarget::Sidetone, 2, true);
  audio_consumer_key(0xEA, 0);
  assert(configuration.sidetone_volume == 0 && !configuration.sidetone_muted);
  set_audio_volume(AudioVolumeTarget::Sidetone, 100, false);
  s_tone(0, 700, 0);
  assert(M5.Speaker.frequency == 700 && M5.Speaker.duration == UINT32_MAX && M5.Speaker.channel == 0);
  assert(delayed_ms == 0);
  s_noTone(0);
  assert(M5.Speaker.stops == 1);
  s_tone(0, 900, 200);
  assert(M5.Speaker.frequency == 900 && M5.Speaker.duration == 200 && delayed_ms == 200);
  M5.Speaker.tone_ok = false;
  s_tone(0, 700, 0);
  assert(log_port.errors == 1);
  log_port.errors = 0;
  #else
  assert(!M5.applied.internal_spk && M5.Speaker.begins == 0);
  s_tone(0, 700, 0);
  s_tone(0, 900, 200);
  s_noTone(0);
  assert(M5.Speaker.tones == 0 && M5.Speaker.stops == 0 && delayed_ms == 200);
  #endif
  assert(!M5.applied.internal_mic);
  assert(!M5.applied.internal_imu && !M5.applied.internal_rtc && !M5.applied.output_power);
  assert(M5.applied.external_display_value == 0);
  assert(lcd.rotation == LCD_ROTATION && lcd.brightness == 128);
  for (int failure = 0; failure < 3; ++failure) {
    M5.board = failure == 0 ? m5::board_t::board_unknown : m5::board_t::board_M5StackCore2;
    lcd.has_touch = failure != 1;
    lcd.w = failure == 2 ? 240 : 320;
    bool rejected = false;
    try { initialize_core2_board(); } catch (const std::runtime_error &) { rejected = true; }
    assert(rejected);
  }
  assert(log_port.errors == 3);
  #ifdef FEATURE_CORE2_AUDIO
  lcd.w = 320;
  M5.Speaker.begin_ok = false;
  bool failed = false;
  try { initialize_core2_board(); } catch (const std::runtime_error &) { failed = true; }
  assert(failed && log_port.errors == 4);
  #endif
  BUTTON_CHECKS
  puts("PASS: Core2 profile, two TX/PTT pairs, distinct GPIO, internal peripheral exclusions");
}
'@
$temp = Join-Path ([IO.Path]::GetTempPath()) ("core2-gpio-test-" + [guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $temp
$source = Join-Path $temp 'test.cpp'
$exe = Join-Path $temp 'test.exe'
$obj = Join-Path $temp 'test.obj'
try {
    $featureSelection = "`n#ifdef TEST_SILENT`n#undef FEATURE_CORE2_AUDIO`n#endif`n"
    $body = $features + $featureSelection + $pins + "`n" + $rows + "`n" + $mocks + "`n" + $inputs + "`n" + $paddleRead +
        "`n" + $beaconToggle + "`n" + $outputHelpers + "`nvoid initialize_test_outputs() {`n" + $outputInit + "`n}`n" + $portAMode + "`n" + $boardInit + "`n" +
        "#define BUTTON_ASSIGNMENTS`n" + $assignment + "`n" + $layout + "`n" + $cwLayout + "`n" + $buttons + "`n" +
        $buttonMocks + "`n" + $buttonInit + "`n" + $touchRead + "`n" + $buttonProcess + "`n" + $tones + "`n" + $audio + "`n" +
        $checks.Replace('BUTTON_CHECKS', $buttonChecks)
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual Studio C++ tools not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    foreach ($rows in @('default', 'silent', 4, 8, 0, 9)) {
        $selection = if ($rows -eq 'default') { "#define TEST_DEFAULT_ROWS`n" } elseif ($rows -eq 'silent') { "#define TEST_SILENT`n" } else { "#define BUTTON_ROWS $rows`n" }
        Set-Content -LiteralPath $source -Value ($sourceText + "`n" + $selection + $body) -Encoding ASCII
        & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W3 /I`"$root\components\bt_keyboard\src`" /Fo`"$obj`" /Fe`"$exe`" `"$source`""
        if ($rows -in @(0, 9)) {
            if ($LASTEXITCODE -eq 0) { throw "Invalid Core2 row count $rows compiled" }
            Write-Host "PASS: invalid Core2 BUTTON_ROWS=$rows rejected"
            continue
        }
        if ($LASTEXITCODE -ne 0) { throw 'Core2 host compilation failed' }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw 'Core2 GPIO/button assertions failed' }
    }
} finally {
    foreach ($file in @($source, $exe, $obj)) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    Remove-Item -LiteralPath $temp
}
