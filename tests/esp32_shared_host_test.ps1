$ErrorActionPreference = 'Stop'
$main = Get-Content (Join-Path (Split-Path $PSScriptRoot -Parent) 'main\main.cpp') -Raw
$inputs = [regex]::Match($main, '(?s)// ESP32 local pin interrupt setup.*?(?=//-{10,}\s*#if defined\(FEATURE_MCP23017_EXPANDER\))').Value
if (!$inputs) { throw 'ESP32 input setup not found' }
$gpsHelpers = [regex]::Match($main, '(?s)bool gps_data_fresh\(.*?(?=#define GRID_BEEP_HOLDOFF_MS)').Value
if (!$gpsHelpers) { throw 'Shared GPS helpers not found' }
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
#include <string>
#include <vector>
#define USE_KEY_PIN_INTERRUPTS
#define HARDWARE_ESP32_DEV
#define FEATURE_TFT_DISPLAY
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
constexpr int LCD_COLUMNS = 10, LCD_SCROLL_MSG = 3;
int lcd_status = LCD_SCROLL_MSG;
byte lcd_scroll_flag = 0, lcd_scroll_buffer_dirty = 0;
void lcd_scroll_box_clear() {}
struct Glyph { char c; int x, y; };
struct Display {
  int fills = 0;
  std::vector<Glyph> glyphs;
  void setTextDatum(int) {}
  void setFreeFont(int) {}
  void setTextColor(int) {}
  void fillRect(int, int, int, int, int) { ++fills; }
  void drawChar(char c, int x, int y, int) { glyphs.push_back({c, x, y}); }
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
  lcd_scroll_buffer[0] = "ABC";
  assert(!tft_draw_pending_scroll_chars(2));
  assert(lcd.glyphs.size() == 2 && lcd.fills == 0);
  assert(tft_draw_pending_scroll_chars(2));
  assert(lcd.glyphs.size() == 3 && lcd.glyphs.back().c == 'C');
  assert(tft_draw_pending_scroll_chars(2) && lcd.glyphs.size() == 3);
  lcd_scroll_buffer[1] = "D";
  assert(tft_draw_pending_scroll_chars(2));
  assert(lcd.glyphs.size() == 4 && lcd.glyphs.back().y == SCROLL_BOX_ROW2);
  lcd_scroll_repaint_row = 0;
  for (int row = 0; row < LCD_ROWS; ++row) {
    const int fills = lcd.fills;
    assert(!tft_draw_pending_scroll_chars(2));
    assert(lcd.fills == fills + 1 && lcd_scroll_repaint_row == row + 1);
  }
  assert(tft_draw_pending_scroll_chars(2));
  reset_display_scroll_text();
  for (int i = 0; i < 24; ++i) display_scroll_print_char('X');
  display_scroll_print_char(' ');
  assert(lcd_scroll_row_pointer > 0 && lcd_scroll_holding_space == 1);
  reset_display_scroll_text();
  assert(lcd_scroll_row_pointer == 0 && lcd_scroll_column_pointer == 0);
  assert(lcd_scroll_holding_space == 0 && lcd_scroll_flag == 0);
  assert(lcd_scroll_drawn_row == 0 && lcd_scroll_drawn_col == 0);
  for (const auto &row : lcd_scroll_buffer) assert(row.empty());
  display_scroll_print_char('N');
  assert(lcd_scroll_buffer[0] == "N" && lcd_scroll_column_pointer == 1);
  assert(lcd_scroll_buffer[1].empty());
  lcd.glyphs.clear();
  while (!tft_draw_pending_scroll_chars(2)) {}
  assert(lcd.glyphs.size() == 1 && lcd.glyphs[0].c == 'N');
  assert(lcd.glyphs[0].x == SCROLL_TEXT_LEFT_SIDE && lcd.glyphs[0].y == SCROLL_BOX_ROW1);
  puts("PASS: ESP32 input levels/edges, GPS freshness/holdover, incremental TFT draw and row repaint");
}
'@
$temp = Join-Path ([IO.Path]::GetTempPath()) ("esp32-shared-test-" + [guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $temp
$source = Join-Path $temp 'test.cpp'
$exe = Join-Path $temp 'test.exe'
$obj = Join-Path $temp 'test.obj'
try {
    Set-Content -LiteralPath $source -Value ("#include <initializer_list>`n" + $mock + $inputs + $gpsHelpers + $scroll + $print + $checks) -Encoding ASCII
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual Studio C++ tools not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    foreach ($defines in @('', '/DFEATURE_STRAIGHT_KEY')) {
        & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W3 $defines /Fo`"$obj`" /Fe`"$exe`" `"$source`""
        if ($LASTEXITCODE -ne 0) { throw "ESP32 host compilation failed: $defines" }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw "ESP32 host assertions failed: $defines" }
    }
} finally {
    foreach ($file in @($source, $exe, $obj)) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    Remove-Item -LiteralPath $temp
}
