$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$main = Get-Content (Join-Path $root 'main\main.cpp') -Raw
$inputs = [regex]::Match($main, '(?s)// ESP32 local pin interrupt setup.*?(?=//-{10,}\s*#if defined\(FEATURE_MCP23017_EXPANDER\))').Value
if (!$inputs) { throw 'ESP32 input setup not found' }
$scroll = [regex]::Match($main, '(?s)#ifdef FEATURE_TFT_DISPLAY\r?\nint32_t tft_scroll_row_y.*?(?=\r?\nvoid service_display\(\))').Value
if (!$scroll) { throw 'Shared TFT scroll helpers not found' }
$print = [regex]::Match($main, '(?s)byte lcd_scroll_column_pointer = 0;.*?(?=\r?\n#endif //FEATURE_DISPLAY)').Value
if (!$print) { throw 'Scroll reset and insertion functions not found' }
if ($main -notmatch 'gps_serial_port\.setRxBufferSize\(1024\) != 1024' -or
    $main -notmatch '(?s)gps_serial_port\.setRxBufferSize\(1024\).*?gps_serial_port\.begin\(configuration\.gps_baud') {
    throw 'ESP32 GPS RX buffer must be configured before begin'
}
$mock = @'
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "keyer_gps.h"
#define USE_KEY_PIN_INTERRUPTS
#define HARDWARE_ESP32_DEV
#ifndef TEST_TEXT_LCD
#define FEATURE_TFT_DISPLAY
#endif
#define IRAM_ATTR
using gpio_num_t = int;
constexpr int paddle_left = 4, paddle_right = 5, pin_straight_key = 6;
constexpr int GPIO_MODE_INPUT = 1, GPIO_PULLUP_ONLY = 2, GPIO_INTR_ANYEDGE = 3;
int levels[8] = {};
int directions[8] = {}, pulls[8] = {};
bool paddle_left_state = false, paddle_right_state = false, straight_key_state = false;
int gpio_install_isr_service(int) { return 0; }
int gpio_set_direction(int pin, int mode) { directions[pin] = mode; return 0; }
int gpio_set_pull_mode(int pin, int mode) { pulls[pin] = mode; return 0; }
int gpio_set_intr_type(int, int) { return 0; }
int gpio_isr_handler_add(int, void (*)(void *), void *) { return 0; }
int gpio_get_level(int pin) { return levels[pin]; }
constexpr int LCD_ROWS = 5, FONT_HEIGHT = 16, COLUMN_WIDTH = 8;
constexpr int SCROLL_BOX_ROW1 = 16, SCROLL_BOX_ROW2 = 32, SCROLL_BOX_ROW3 = 48;
constexpr int SCROLL_BOX_ROW4 = 64, SCROLL_BOX_ROW5 = 80;
constexpr int SCROLL_BOX_LEFT_SIDE = 0, SCROLL_TEXT_LEFT_SIDE = 4, SCROLL_BOX_WIDTH = 100;
constexpr int GFXFF = 1, SCROLL_BOX_DATUM = 0, SCROLL_BOX_FONT = 1, TFT_WHITE = 1, TFT_BLACK = 0;
using byte = uint8_t;
struct Text : std::string {
  using std::string::operator=;
  void concat(char c) { push_back(c); }
  char charAt(size_t index) const { return index < size() ? at(index) : 0; }
} lcd_scroll_buffer[LCD_ROWS];
byte lcd_scroll_repaint_row = LCD_ROWS, lcd_scroll_drawn_row = 0, lcd_scroll_drawn_col = 0;
byte lcd_scroll_repaint_col = 255;
constexpr int LCD_COLUMNS = 10, LCD_SCROLL_MSG = 3;
uint16_t lcd_scroll_colors[LCD_ROWS][LCD_COLUMNS] = {};
uint16_t lcd_scroll_holding_color = TFT_WHITE;
bool lcd_scroll_needs_clear = false;
int lcd_status = LCD_SCROLL_MSG;
byte lcd_scroll_flag = 0, lcd_scroll_buffer_dirty = 0;
int scroll_clears = 0;
void lcd_scroll_box_clear() { ++scroll_clears; }
void display_scroll_print_char_color(char charin, uint16_t color);
struct Glyph { char c; int x, y; uint16_t color; };
struct Display {
  int fills = 0;
  uint16_t color = TFT_WHITE;
  std::vector<Glyph> glyphs;
  void setTextDatum(int) {}
  void setFreeFont(int) {}
  void setTextColor(uint16_t value) { color = value; }
  void fillRect(int, int, int, int, int) { ++fills; }
  void drawChar(char c, int x, int y, int) { glyphs.push_back({c, x, y, color}); }
} lcd;
'@
$checks = @'
int main() {
  for (int left : {0, 1}) for (int right : {0, 1}) {
    for (int key : {0, 1}) {
      levels[paddle_left] = left; levels[paddle_right] = right;
      levels[pin_straight_key] = key;
      init_ESP32_GPIO_key_pins();
      assert(paddle_left_state == bool(left) && paddle_right_state == bool(right));
      assert(directions[paddle_left] == GPIO_MODE_INPUT && pulls[paddle_left] == GPIO_PULLUP_ONLY);
      assert(directions[paddle_right] == GPIO_MODE_INPUT && pulls[paddle_right] == GPIO_PULLUP_ONLY);
#ifdef FEATURE_STRAIGHT_KEY
      assert(straight_key_state == bool(key));
      assert(directions[pin_straight_key] == GPIO_MODE_INPUT && pulls[pin_straight_key] == GPIO_PULLUP_ONLY);
      levels[pin_straight_key] = !key; straight_key_intr_handler(nullptr);
      assert(straight_key_state == !bool(key));
#endif
      levels[paddle_left] = !left; left_paddle_intr_handler(nullptr);
      levels[paddle_right] = !right; right_paddle_intr_handler(nullptr);
      assert(paddle_left_state == !bool(left) && paddle_right_state == !bool(right));
    }
  }
  assert(!gps_data_fresh(false, 0));
  assert(gps_data_fresh(true, 4999) && !gps_data_fresh(true, 5000));
  assert(!gps_location_held(false, 0));
  assert(gps_location_held(true, 59999) && !gps_location_held(true, 60000));
  assert(!gps_location_held(true, UINT32_MAX));
#ifdef FEATURE_TFT_DISPLAY
  lcd_scroll_buffer[0] = "ABC";
  lcd_scroll_colors[0][0] = 7;
  lcd_scroll_colors[0][1] = 8;
  lcd_scroll_colors[0][2] = 9;
  assert(!tft_draw_pending_scroll_chars(2));
  assert(lcd.glyphs.size() == 2 && lcd.fills == 0);
  assert(tft_draw_pending_scroll_chars(2));
  assert(lcd.glyphs.size() == 3 && lcd.glyphs.back().c == 'C');
  assert(lcd.glyphs[0].color == 7 && lcd.glyphs[1].color == 8 && lcd.glyphs[2].color == 9);
  assert(tft_draw_pending_scroll_chars(2) && lcd.glyphs.size() == 3);
  lcd_scroll_buffer[1] = "D";
  assert(tft_draw_pending_scroll_chars(2));
  assert(lcd.glyphs.size() == 4 && lcd.glyphs.back().y == SCROLL_BOX_ROW2);
  lcd_scroll_repaint_row = 0;
  lcd_scroll_repaint_col = 255;
  lcd_scroll_needs_clear = true;
  const int clears_before = scroll_clears;
  for (int row = 0; row < LCD_ROWS; ++row) {
    const int fills = lcd.fills;
    assert(!tft_draw_pending_scroll_chars(2));
    assert(lcd.fills == fills + 1 && lcd_scroll_repaint_row == row);
    assert(lcd_scroll_repaint_col == 0);
    const size_t glyphs_before = lcd.glyphs.size();
    const size_t length = lcd_scroll_buffer[row].size();
    const size_t passes = length == 0 ? 1 : (length + 1) / 2;
    for (size_t pass = 0; pass < passes; ++pass) {
      const size_t before = lcd.glyphs.size();
      assert(!tft_draw_pending_scroll_chars(2));
      assert(lcd.glyphs.size() - before <= 2 && lcd.fills == fills + 1);
    }
    assert(lcd_scroll_repaint_row == row + 1 && lcd_scroll_repaint_col == 255);
    assert(lcd.glyphs.size() - glyphs_before == length);
  }
  assert(scroll_clears == clears_before + 1 && !lcd_scroll_needs_clear);
  assert(tft_draw_pending_scroll_chars(2));
  assert(lcd_scroll_drawn_row == 1 && lcd_scroll_drawn_col == 1);
#endif
  reset_display_scroll_text();
  for (int i = 0; i < 24; ++i) display_scroll_print_char('X');
  display_scroll_print_char(' ');
  assert(lcd_scroll_row_pointer > 0 && lcd_scroll_holding_space == 1);
  reset_display_scroll_text();
  assert(lcd_scroll_row_pointer == 0 && lcd_scroll_column_pointer == 0);
  assert(lcd_scroll_holding_space == 0 && lcd_scroll_flag == 0);
  assert(lcd_scroll_drawn_row == 0 && lcd_scroll_drawn_col == 0);
#ifdef FEATURE_TFT_DISPLAY
  assert(lcd_scroll_repaint_row == 0 && lcd_scroll_repaint_col == 255);
  for (const auto &row : lcd_scroll_colors) for (uint16_t color : row) assert(color == TFT_WHITE);
#endif
  for (const auto &row : lcd_scroll_buffer) assert(row.empty());
  display_scroll_print_char('N');
  assert(lcd_scroll_buffer[0] == "N" && lcd_scroll_column_pointer == 1);
  assert(lcd_scroll_buffer[1].empty());
#ifdef FEATURE_TFT_DISPLAY
  lcd.glyphs.clear();
  bool complete = false;
  for (int pass = 0; pass < 2 * LCD_ROWS + 2 && !complete; ++pass)
    complete = tft_draw_pending_scroll_chars(2);
  assert(complete);
  assert(lcd.glyphs.size() == 1 && lcd.glyphs[0].c == 'N');
  assert(lcd.glyphs[0].x == SCROLL_TEXT_LEFT_SIDE && lcd.glyphs[0].y == SCROLL_BOX_ROW1);
  assert(lcd.glyphs[0].color == TFT_WHITE);
#endif
  display_scroll_print_char_color(' ', 7);
  display_scroll_print_char_color('R', 8);
  assert(lcd_scroll_buffer[0] == "N R" && lcd_scroll_column_pointer == 3);
#ifdef FEATURE_TFT_DISPLAY
  assert(lcd_scroll_colors[0][1] == 7 && lcd_scroll_colors[0][2] == 8);
#endif
  reset_display_scroll_text();
  constexpr int total_chars = 2 * LCD_ROWS * LCD_COLUMNS + 7;
  for (int i = 0; i < total_chars; ++i)
    display_scroll_print_char_color(static_cast<char>('A' + i % 26), static_cast<uint16_t>(16 + i));
  assert(lcd_scroll_row_pointer == LCD_ROWS - 1 && lcd_scroll_column_pointer == 7);
  assert(lcd_scroll_flag == 1 && lcd_scroll_buffer_dirty == 1);
  const int first_retained = total_chars - ((LCD_ROWS - 1) * LCD_COLUMNS + 7);
  for (int row = 0; row < LCD_ROWS; ++row) {
    const int length = row == LCD_ROWS - 1 ? 7 : LCD_COLUMNS;
    assert(lcd_scroll_buffer[row].size() == static_cast<size_t>(length));
    for (int col = 0; col < length; ++col) {
      const int index = first_retained + row * LCD_COLUMNS + col;
      assert(lcd_scroll_buffer[row][col] == 'A' + index % 26);
#ifdef FEATURE_TFT_DISPLAY
      assert(lcd_scroll_colors[row][col] == 16 + index);
#endif
    }
  }
#ifdef FEATURE_TFT_DISPLAY
  puts("PASS: ESP32 input levels/edges, GPS freshness/holdover, incremental TFT draw and row repaint");
#else
  puts("PASS: ESP32 input levels/edges, GPS freshness/holdover, legacy text-LCD scroll insertion/reset");
#endif
}
'@
$temp = Join-Path ([IO.Path]::GetTempPath()) ("esp32-shared-test-" + [guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $temp
$source = Join-Path $temp 'test.cpp'
$exe = Join-Path $temp 'test.exe'
$obj = Join-Path $temp 'test.obj'
try {
    Set-Content -LiteralPath $source -Value ("#include <initializer_list>`n" + $mock + $inputs + $scroll + $print + $checks) -Encoding ASCII
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual Studio C++ tools not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    $gpsObj = Join-Path $temp 'keyer_gps.obj'
    & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W4 /c /Fo`"$gpsObj`" `"$root\main\keyer_gps.cpp`""
    if ($LASTEXITCODE -ne 0) { throw 'GPS module compilation failed' }
    & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W4 /I`"$root\main`" /Fo`"$obj`" /Fe`"$exe`" `"$PSScriptRoot\keyer_gps_host_test.cpp`" `"$gpsObj`""
    if ($LASTEXITCODE -ne 0) { throw 'GPS regression compilation failed' }
    & $exe
    if ($LASTEXITCODE -ne 0) { throw 'GPS regression assertions failed' }
    foreach ($defines in @('', '/DFEATURE_STRAIGHT_KEY', '/DTEST_TEXT_LCD /DFEATURE_STRAIGHT_KEY')) {
        & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W3 /I`"$root\main`" $defines /Fo`"$obj`" /Fe`"$exe`" `"$source`" `"$gpsObj`""
        if ($LASTEXITCODE -ne 0) { throw "ESP32 host compilation failed: $defines" }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw "ESP32 host assertions failed: $defines" }
    }
} finally {
    foreach ($file in @($source, $exe, $obj, (Join-Path $temp 'keyer_gps.obj'))) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    Remove-Item -LiteralPath $temp
}
