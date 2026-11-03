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
$buttonInit = [regex]::Match($main, '(?s)void initialize_buttons\(\) \{.*?\r?\n\}').Value
$touchRead = [regex]::Match($main, '(?s)bool read_core2_touch\(.*?\r?\n\}').Value
$buttonProcess = [regex]::Match($main, '(?s)void process_buttons\(\) \{.*?\r?\n\}').Value
$inputs = [regex]::Match($main, '(?s)// ESP32 local pin interrupt setup.*?(?=//-{10,}\s*#if defined\(FEATURE_MCP23017_EXPANDER\))').Value
if (!$assignment -or !$buttonInit -or !$touchRead -or !$buttonProcess) { throw 'Core2 button code not found' }
$boardInit = [regex]::Match($main, '(?s)void initialize_core2_board\(\) \{.*?\r?\n\}(?=\r?\n#endif)').Value
if (!$boardInit) { throw 'Core2 board initialization not found' }
$sourceText = @'
#include <array>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <cstdint>
#include <cstring>
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
namespace m5 { enum class board_t { board_M5StackCore2, board_unknown }; }
struct BoardConfig {
  m5::board_t fallback_board = m5::board_t::board_unknown;
  bool internal_spk = true, internal_mic = true, internal_imu = true, internal_rtc = true;
  int external_display_value = 255;
  bool output_power = true;
};
struct Board {
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
struct Log { int errors = 0; void println(const char *) { ++errors; } } log_port;
Log *debug_serial_port = &log_port;
#define F(x) x
#define ESP_FAIL 1
#define ESP_ERROR_CHECK(x) do { if (x) throw std::runtime_error("board init failed"); } while (0)
'@
$buttonMocks = @'
int created = 0, refreshed = -1, queued = -1, action = -1;
unsigned long last_active_time = 0;
bool popup_active = false;
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
  #ifdef TEST_DEFAULT_ROWS
  static_assert(BUTTON_ROWS == 4);
  #endif
  static_assert(MAX_TX_PORTS == 2);
  for (int level : {0, 1}) {
    levels[pin_straight_key] = level;
    init_ESP32_GPIO_key_pins();
    assert(directions[pin_straight_key] == GPIO_MODE_INPUT && pulls[pin_straight_key] == 0);
    assert(straight_key_state == bool(level));
    assert(pulls[paddle_left] == GPIO_PULLUP_ONLY && pulls[paddle_right] == GPIO_PULLUP_ONLY);
  }
  const std::array<int, 7> pins = {paddle_left, paddle_right, pin_straight_key,
      tx_key_line_1, ptt_tx_1, tx_key_line_2, ptt_tx_2};
  const std::array<int, 18> reserved = {0, 1, 2, 3, 4, 5, 12, 14, 15, 16, 17, 18, 21, 22, 23, 34, 38, 39};
  for (size_t i = 0; i < pins.size(); ++i) {
    for (size_t j = i + 1; j < pins.size(); ++j) assert(pins[i] != pins[j]);
    for (int pin : reserved) assert(pins[i] != pin);
  }
  for (int pin : {tx_key_line_1, ptt_tx_1, tx_key_line_2, ptt_tx_2}) assert(pin > 0 && pin < 34);
  assert(paddle_left == 32 && paddle_right == 27 && pin_straight_key == 35);
  assert(tx_key_line_1 == 25 && ptt_tx_1 == 26 && tx_key_line_2 == 33 && ptt_tx_2 == 19);
  assert(ptt_input_pin == 0);
  assert(sidetone_line == 0 && audio_enable == 0 && GPS_RX_PIN == 0);
  assert(tx_key_line_3 == 0 && tx_key_line_4 == 0 && tx_key_line_5 == 0 && tx_key_line_6 == 0);
  assert(ptt_tx_3 == 0 && ptt_tx_4 == 0 && ptt_tx_5 == 0 && ptt_tx_6 == 0);
  initialize_core2_board();
  assert(!M5.applied.internal_spk && !M5.applied.internal_mic);
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
    $body = $features + "`n" + $pins + "`n" + $mocks + "`n" + $inputs + "`n" + $boardInit + "`n" +
        "#define BUTTON_ASSIGNMENTS`n" + $assignment + "`n" + $layout + "`n" + $buttons + "`n" +
        $buttonMocks + "`n" + $buttonInit + "`n" + $touchRead + "`n" + $buttonProcess + "`n" +
        $checks.Replace('BUTTON_CHECKS', $buttonChecks)
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual Studio C++ tools not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    foreach ($rows in @('default', 4, 8, 0, 9)) {
        $selection = if ($rows -eq 'default') { "#define TEST_DEFAULT_ROWS`n" } else { "#define BUTTON_ROWS $rows`n" }
        Set-Content -LiteralPath $source -Value ($sourceText + "`n" + $selection + $body) -Encoding ASCII
        & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W3 /Fo`"$obj`" /Fe`"$exe`" `"$source`""
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
