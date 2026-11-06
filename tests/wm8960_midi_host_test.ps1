param([switch]$CwEncodersOnly)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$midi = Get-Content (Join-Path $root 'main\wm8960_midi.cpp') -Raw
$header = Get-Content (Join-Path $root 'main\wm8960_midi.h') -Raw
$audio = Get-Content (Join-Path $root 'main\wm8960_audio.cpp') -Raw
$main = Get-Content (Join-Path $root 'main\main.cpp') -Raw
$midi = [regex]::Replace($midi, '(?m)^#include .*\r?$', '')
$header = [regex]::Replace($header, '(?m)^#include .*\r?$', '')

function Get-CppFunction([string]$text, [string]$name) {
    $match = [regex]::Match($text, "(?m)^[\w:*& `t]+\b$name\([^;]*?\)\s*\{")
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

$setup = Get-CppFunction $main 'setup_esp'
$setup = [regex]::Replace($setup, '(?m)//.*$', '')
$displayInit = $setup.IndexOf('initialize_display();')
$gpsInit = $setup.IndexOf('initialize_gps_port();')
$settingsInit = $setup.IndexOf('check_eeprom_for_initialization();')
if ($settingsInit -lt 0 -or $displayInit -le $settingsInit -or $gpsInit -le $displayInit) {
    throw 'GPS startup must follow saved settings and display initialization: GPS reports its baud rate on the LCD'
}
if ([regex]::Matches($setup, '\binitialize_gps_port\(\);').Count -ne 1) {
    throw 'GPS must initialize exactly once during startup'
}
if ($main -notmatch '(?s)void check_gps\(void \* pvParameters\).*?for\s*\(;;\)\s*\{\s*if \(gps_updates_paused\(\)\)' -or
    $main -notmatch 'void check_gps\(\)\s*\{\s*if \(1\)\s*\{\s*if \(gps_updates_paused\(\)\) return;' -or
    $main -notmatch 'while \(!gps_updates_paused\(\) && gps_serial_port.available\(\) > 0\)' -or
    $main -notmatch 'void process_gps\(\) \{[^\r\n]*\r?\n\s*if \(gps_updates_paused\(\)\) return;') {
    throw 'GPS task, foreground/core1 parsing, and grid processing must pause during PTT/keying'
}

$audioFunctions = @(
    'advance_mute_ramp', 'apply_mute_ramp', 'usb_audio_muted_for_keyer',
    'microphone_muted_for_gate', 'wm8960_audio_set_local_ptt', 'wm8960_audio_set_external_ptt', 'wm8960_audio_external_ptt',
    'next_tone_sample', 'apply_tone_edge', 'wm8960_audio_tone_active', 'wm8960_audio_feedback_pending', 'wm8960_audio_stop_tone',
    'wm8960_audio_set_midi_input', 'render_i2s_output',
    'wm8960_audio_set_output_route', 'wm8960_audio_output_route',
    'wm8960_audio_output_route_name', 'wm8960_audio_configure_rx',
    'wm8960_audio_rx_status', 'wm8960_audio_service_rx', 'wm8960_audio_set_levels',
    'wm8960_audio_trace_keyer'
) | ForEach-Object { Get-CppFunction $audio $_ }
$terminalWriteStart = $audio.IndexOf('size_t Wm8960TerminalSerial::write(const uint8_t *buffer, size_t size)')
if ($terminalWriteStart -lt 0) { throw 'Terminal writer not found' }
$terminalWrite = Get-CppFunction ($audio.Substring($terminalWriteStart)) 'write'
$terminalWrite = $terminalWrite.Replace('Wm8960TerminalSerial::write','terminal_write')
$terminalWaitTest = @'
namespace terminal_wait_test {
bool locked = false, running_ = true, usb_registration_succeeded = true, write_error = false;
struct { int mutex; } USB;
struct CoreMutex {
  CoreMutex(int *, bool) { assert(!locked); locked = true; }
  ~CoreMutex() { locked = false; }
  operator bool() const { return true; }
};
uint32_t waited_ms = 0;
unsigned services = 0;
CwRxDecoder decoder;
uint32_t millis() { return waited_ms; }
bool tud_cdc_n_connected(int) { return true; }
size_t tud_cdc_n_write(int,const uint8_t *,size_t size) { return waited_ms >= 600 ? size : 0; }
void tud_cdc_n_write_flush(int) {}
void setWriteError() { write_error = true; }
void yield() { waited_ms += 10; }
void service_usb_cw_decoder() {
  assert(!locked);
  int16_t samples[960]{};
  decoder.submit(samples,480,0);
  decoder.process(1280,[](long) {});
  ++services;
}
'@ + "`n" + $terminalWrite + @'

void run() {
  assert(decoder.configure(50,700,20));
  decoder.process(1280,[](long) {});
  const uint8_t text[] = {'R','X'};
  assert(terminal_write(text,2) == 2);
  assert(waited_ms == 600 && services == 60 && !write_error);
  assert(decoder.dropped() == 0 && decoder.pending_samples() == 0);
  puts("PASS: 600 ms CDC backpressure services RX outside USB mutex with zero PCM drops");
}
}
'@
$clearTest = @'
namespace scroll_clear_test {
constexpr int SCROLL_BOX_LEFT_SIDE = 2, SCROLL_BOX_TOP = 39;
constexpr int SCROLL_BOX_WIDTH = 476, SCROLL_BOX_HEIGHT = 138, TFT_BLACK = 0;
uint32_t pending_us = 0, maximum_us = 0;
unsigned services = 0;
bool rows[SCROLL_BOX_HEIGHT]{};
struct {
  void fillRect(int x,int y,int w,int h,int color) {
    assert(x == SCROLL_BOX_LEFT_SIDE && w == SCROLL_BOX_WIDTH && color == TFT_BLACK);
    assert(y >= SCROLL_BOX_TOP && y+h <= SCROLL_BOX_TOP+SCROLL_BOX_HEIGHT && h <= 12);
    for (int row=y-SCROLL_BOX_TOP; row<y-SCROLL_BOX_TOP+h; ++row) rows[row] = true;
    pending_us += h*2000;
  }
  void fillRoundRect(int x,int y,int w,int h,int radius,int color) {
    assert(radius == 6 && h == 12);
    fillRect(x,y,w,h,color);
  }
} lcd;
void service_usb_cw_decoder() {
  maximum_us = std::max(maximum_us,pending_us);
  assert(pending_us <= 24000);
  pending_us = 0;
  ++services;
}
'@ + "`n" + (Get-CppFunction $main 'tft_clear_scroll_box_pixels') + @'

void run() {
  tft_clear_scroll_box_pixels();
  for (bool row : rows) assert(row);
  assert(services > 10 && maximum_us == 24000 && pending_us == 0);
  puts("PASS: full scroll clear services RX between strips, at most 24 ms simulated blocking");
}
}
'@
$toneFunction = Get-CppFunction $main 's_tone'
$keyboardScope = [regex]::Match($main, '(?s)struct KeyboardSendingScope \{.*?\r?\n\};').Value
if (!$keyboardScope) { throw 'Keyboard sending scope not found' }
$pttTailFunction = [regex]::Match($main, '(?ms)^void check_ptt_tail\(\)\s*\{.*?^\}').Value
if (!$pttTailFunction) { throw 'PTT tail function not found' }
$txKeyFunction = [regex]::Match($main, '(?ms)^void tx_and_sidetone_key \(int state\)\s*\{.*?^\}').Value
if (!$txKeyFunction) { throw 'TX key function not found' }
if ([regex]::Matches((Get-CppFunction $main 'service_send_buffer'),
    'send_char\(send_buffer_array\[0\],[^,;]+,send_buffer_from_keyboard\[0\]\)').Count -ne 3 -or
    (Get-CppFunction $main 'service_keyboard_commands') -notmatch 'add_to_send_buffer\(keystroke, true\)' -or
    $main -notmatch 'send_buffer_from_keyboard\[serial_buffer_position_to_overwrite\] = false;') {
    throw 'Keyboard source must follow ordinary/prosign text through the shared buffer and WinKey overwrites'
}
$keyboardPttTest = ''
$keyboardPttRuns = ''
foreach ($wk in 0..1) {
    foreach ($policy in 0..2) {
        $ns = "keyboard_ptt_${wk}_${policy}"
        $keyboardPttRuns += "  ${ns}::run();`n"
        $keyboardPttTest += "`nnamespace $ns {`n"
        if ($wk) { $keyboardPttTest += "#define FEATURE_WINKEY_EMULATION`n" }
        if ($policy -gt 0) { $keyboardPttTest += "#define OPTION_INCLUDE_PTT_TAIL_FOR_MANUAL_SENDING`n" }
        if ($policy -eq 2) { $keyboardPttTest += "#define OPTION_EXCLUDE_PTT_HANG_TIME_FOR_MANUAL_SENDING`n" }
        $keyboardPttTest += @'
constexpr int MANUAL_SENDING = 0, AUTOMATIC_SENDING = 1, KEYER_NORMAL = 0;
constexpr int KEYER_COMMAND_MODE = 2, CW = 0, HELL = 1, AMERICAN_MORSE = 2;
constexpr int OMIT_LETTERSPACE = 1, SIDETONE_ON = 1, SIDETONE_PADDLE_ONLY = 2;
constexpr int SENDING_DIT = 1, SENDING_DAH = 3;
constexpr int tx_key_line_active_state = 1, tx_key_line_inactive_state = 0;
constexpr int ptt_input_pin = 0, ptt_input_pin_active_state = 1, winkey_xon_threshold = 2;
constexpr int winkey_xoff_threshold = 7, send_buffer_size = 8, length_letterspace = 3;
bool keyboard_sending = false, keyboard_ptt_hang = false;
bool key_state = false, ptt_line_activated = false, key_tx = true;
int sending_mode = MANUAL_SENDING, last_sending_mode = MANUAL_SENDING;
int keyer_machine_mode = KEYER_NORMAL, char_send_mode = CW, being_sent = SENDING_DIT;
int current_tx_key_line = 0, first_extension_time = 0;
int manual_ptt_invoke = 0, manual_ptt_invoke_ptt_input_pin = 0;
bool winkey_host_open = false, winkey_xoff = false;
int winkey_sending = 0, winkey_session_ptt_tail = 10;
byte send_buffer_array[send_buffer_size]{}, send_buffer_bytes = 0;
bool send_buffer_from_keyboard[send_buffer_size]{};
float ptt_hang_time_wordspace_units = 1;
unsigned long now_ms = 0, ptt_time = 0;
struct {
  int wpm = 20, wpm_command_mode = 20, length_wordspace = 7, sidetone_mode = 0, hz_sidetone = 700;
  unsigned int ptt_tail_time[1] = {50};
  int current_tx = 1;
  bool ptt_buffer_hold_active = false;
} configuration;
bool pause_sending_buffer = false;
unsigned long millis() { return now_ms; }
int digitalRead(int) { return 0; }
void ptt_key() { ptt_line_activated = true; ptt_time = millis(); }
void ptt_unkey() { ptt_line_activated = false; }
void service_host_ptt_feedback() {}
void keyer_output_write(int,int) {}
void myDelay(int) {}
void s_tone(int,int,int) {}
void s_noTone(int) {}
void wm8960_midi_key(bool) {}
void wm8960_audio_trace_key_edge(bool,int,int) {}
void winkey_port_write(int,int) {}
void loop_element_lengths(float,float,int) {}
'@ + "`n" + $keyboardScope + "`n" +
            $pttTailFunction + "`n" +
            $txKeyFunction + @'

void send_the_dits_and_dahs(const char *) {
  last_sending_mode = sending_mode;
  tx_and_sidetone_key(1);
  tx_and_sidetone_key(0);
}
void send_char(byte,byte,bool = false);
void add_to_send_buffer(byte,bool = false);
'@ + "`n" + (Get-CppFunction $main 'send_char') + "`n" +
            (Get-CppFunction $main 'add_to_send_buffer') + "`n" +
            (Get-CppFunction $main 'remove_from_send_buffer') + @'

unsigned long release_time(bool keyboard, bool paddle) {
  now_ms = ptt_time = 0;
  key_state = ptt_line_activated = false;
  keyboard_sending = keyboard_ptt_hang = false;
  if (paddle) {
    sending_mode = last_sending_mode = MANUAL_SENDING;
    tx_and_sidetone_key(1); tx_and_sidetone_key(0);
  } else {
    send_char('E',KEYER_NORMAL,keyboard);
    assert(!keyboard_sending);
  }
  while (ptt_line_activated && now_ms < 2000) { ++now_ms; check_ptt_tail(); }
  assert(now_ms < 2000);
  return now_ms;
}
void run() {
  for (bool host : {false,true}) {
    winkey_host_open = host;
    const auto paddle = release_time(false,true);
    unsigned long expected = 420;
#ifdef OPTION_INCLUDE_PTT_TAIL_FOR_MANUAL_SENDING
#ifdef FEATURE_WINKEY_EMULATION
    const unsigned long tail = host ? 280 : 50;
#else
    const unsigned long tail = 50;
#endif
#ifdef OPTION_EXCLUDE_PTT_HANG_TIME_FOR_MANUAL_SENDING
    expected = tail;
#else
    expected += tail;
#endif
#endif
    assert(paddle == expected);
    assert(release_time(true,false) == paddle);
    const auto serial = release_time(false,false);
#ifdef FEATURE_WINKEY_EMULATION
    assert(serial == (host ? 281 : 51));
#else
    assert(serial == 51);
#endif
    release_time(true,false);
    assert(keyboard_ptt_hang);
    tx_and_sidetone_key(1); // Direct automatic keying must not inherit keyboard hang.
    assert(!keyboard_ptt_hang);
    tx_and_sidetone_key(0);
    send_char('|',KEYER_NORMAL,true);
    assert(!keyboard_sending);
    send_char(13,KEYER_NORMAL,true);
    assert(!keyboard_sending);
  }
  add_to_send_buffer('A',true);
  add_to_send_buffer('B');
  add_to_send_buffer('C',true);
  assert(send_buffer_from_keyboard[0] && !send_buffer_from_keyboard[1]);
  remove_from_send_buffer();
  assert(send_buffer_array[0] == 'B' && !send_buffer_from_keyboard[0]);
  assert(send_buffer_array[1] == 'C' && send_buffer_from_keyboard[1]);
  add_to_send_buffer(127); // Backspace, followed by replacement from another source.
  add_to_send_buffer('D');
  assert(send_buffer_array[1] == 'D' && !send_buffer_from_keyboard[1]);
  for (int i = 0; i < send_buffer_size; ++i) add_to_send_buffer('K',true);
  assert(send_buffer_bytes == send_buffer_size);
  add_to_send_buffer('Z');
  assert(send_buffer_array[send_buffer_size-1] == 'K' && send_buffer_from_keyboard[send_buffer_size-1]);
  puts("PASS: typed keyboard PTT matches paddle policy; serial timing and mixed-buffer sources preserved");
}
}
#undef FEATURE_WINKEY_EMULATION
#undef OPTION_INCLUDE_PTT_TAIL_FOR_MANUAL_SENDING
#undef OPTION_EXCLUDE_PTT_HANG_TIME_FOR_MANUAL_SENDING

'@
    }
}
$displayFunction = Get-CppFunction $main 'service_display'
$displayGuardEnd = $displayFunction.IndexOf('#ifdef DEBUG_LOOP')
if ($displayGuardEnd -lt 0) { throw 'Display guard boundary not found' }
$displayGuardTest = @'
namespace display_guard_test {
bool popup_active = false, key_state = false, ptt_line_activated = false;
bool tone = false;
bool wm8960_audio_tone_active() { return tone; }
bool encoder_menu_active() { return false; }
void service_encoder_menu() { assert(false); }
constexpr int STATUS_BAR_Y_CURSOR = 0, MY_DATUM = 0, STATUS_BAR_FONT = 1;
constexpr int ICON_ANCHOR = 0, ICON_SPACING = 1, TFT_RED = 2, TFT_GREEN = 3;
constexpr int TFT_BLACK = 0, LCD_SCROLL_MSG = 3;
int normal_calls = 0, scroll_calls = 0;
int lcd_status = LCD_SCROLL_MSG;
bool lcd_scroll_buffer_dirty = false, lcd_scroll_flag = false;
int lcd_scroll_repaint_row = 0, lcd_scroll_repaint_col = 0;
bool tft_draw_pending_scroll_chars(int budget) {
  assert(budget == 2); ++scroll_calls; return true;
}
struct {
  int draws = 0, color = 0;
  char icon = 0;
  void setTextDatum(int) {}
  void setTextFont(int) {}
  void setTextColor(int foreground, int) { color = foreground; }
  void drawChar(char c, int, int) { ++draws; icon = c; }
} lcd;
'@ + "`n" + (Get-CppFunction $main 'draw_tx_icon') + "`n" +
    $displayFunction.Substring(0,$displayGuardEnd) + "`n++normal_calls;`n}`n" + @'
void run() {
  service_display();
  assert(lcd.icon == 'R' && lcd.color == TFT_GREEN && normal_calls == 1);
  key_state = ptt_line_activated = tone = true; // Ctrl-T holds key and sidetone.
  lcd_scroll_buffer_dirty = true;
  service_display();
  assert(lcd.icon == 'T' && lcd.color == TFT_RED);
  assert(normal_calls == 1 && scroll_calls == 0);
  const int draws = lcd.draws;
  service_display();
  assert(lcd.draws == draws); // No repeated LCD writes while held.
  for (int element = 0; element < 3; ++element) { // Keyboard-sent CW, PTT held across gaps.
    key_state = tone = false;
    service_display();
    key_state = tone = true;
    service_display();
    assert(lcd.icon == 'T' && lcd.draws == draws && normal_calls == 1);
  }
  assert(scroll_calls == 1);
  key_state = tone = false; // Ctrl-U / PTT tail permits bounded text.
  service_display();
  assert(lcd.icon == 'T' && scroll_calls == 1 && normal_calls == 1);
  popup_active = true;
  ptt_line_activated = false;
  tone = true;
  service_display();
  assert(lcd.draws == draws);
  popup_active = false;
  service_display();
  assert(lcd.icon == 'R' && lcd.color == TFT_GREEN && normal_calls == 1);
  tone = false;
  service_display();
  assert(normal_calls == 2);
  puts("PASS: held tune updates cached T/R icon, defers text/LCD work, respects popup");
}
}
'@
if ($toneFunction -notmatch 'wm8960_audio_start_tone\(frequency, duration_ms, mute_release_ms\);\s*if \(duration_ms\)' -or
    $toneFunction -notmatch 'while \(wm8960_audio_feedback_pending\(\)\)' -or
    $toneFunction -notmatch 'millis\(\) - drain_started_ms < 6' -or
    $toneFunction -notmatch 'Error: WM8960 feedback tone stalled' -or
    $main -notmatch '(?s)void service_display\(\) \{\s*#if[^\r\n]*\r?\n\s*#ifdef FEATURE_TFT_DISPLAY\s*draw_tx_icon\(\);\s*#endif\s*if \(key_state \|\| wm8960_audio_tone_active\(\)\) return;\s*if \(ptt_line_activated\) \{.*?tft_draw_pending_scroll_chars\(2\).*?#endif\s*return;') {
    throw 'Display servicing must update the cached T/R icon before deferring during tones/keying; PTT also allows bounded live text'
}
$sine = [regex]::Match($audio, '(?s)constexpr int16_t kSineTable\[64\] = \{.*?\};').Value
if (!$sine) { throw 'Sine table not found' }
$config = [regex]::Match($main, '(?s)struct config_t \{.*?\} configuration;').Value
if (!$config) { throw 'EEPROM configuration structure not found' }
$cli = Get-CppFunction $main 'cli_midi'
$midiStatus = Get-CppFunction $main 'print_midi_status'
$rxCli = Get-CppFunction $main 'cli_rx_audio'
$encoderCli = Get-CppFunction $main 'cli_encoder_assignment'
$scrollFunctions = @('reset_display_scroll_text','display_scroll_print_char',
    'display_scroll_print_char_color','tft_draw_scroll_char','tft_draw_pending_scroll_chars','emit_usb_cw_character') |
    ForEach-Object { Get-CppFunction $main $_ }
$rxHeader = [regex]::Match((Get-Content (Join-Path $root 'main\wm8960_audio.h') -Raw),
    '(?s)enum class Wm8960OutputRoute.*?(?=void wm8960_audio_service\(void\))').Value
$rxGlobals = [regex]::Match($audio, '(?s)CwRxDecoder rx_decoder;.*?(?=__attribute__)').Value
$rxGlobals += "`n" + [regex]::Match($audio, '(?s)namespace \{\r?\nuint32_t rx_service_last_ms.*?\r?\n\}').Value
if (!$rxHeader -or !$rxGlobals) { throw 'RX audio declarations not found' }
if ((Get-CppFunction $audio 'wm8960_audio_service_rx') -notmatch 'rx_decoder\.process\(1280, emit, micros, 2000\)') {
    throw 'RX catch-up must have both sample and wall-clock budgets'
}
if ((Get-CppFunction $main 'myDelay') -notmatch 'service_usb_cw_decoder\(\);') {
    throw 'Cooperative audio waits must also service the RX decoder'
}
if ((Get-CppFunction $main 'service_usb_cw_decoder') -notmatch 'if \(servicing\) return;') {
    throw 'Cooperative RX servicing must reject recursive calls'
}
if ($audio.IndexOf('rx_decoder.submit(packet') -gt $audio.IndexOf('(static_cast<int32_t>(packet[frame * 2]) *')) {
    throw 'Decoder PCM tap must precede playback gain'
}
$encoder = [regex]::Match($main, '(?s)enum class EncoderFunction.*?(?=\r?\n#endif\r?\n\r?\nint chk_rotary_encoder)').Value
if (!$encoder) { throw 'Encoder menu functions not found' }
$encoderCapture = [regex]::Match($main, '(?s)struct EncoderEvent.*?(?=\r?\n#endif)').Value
if (!$encoderCapture) { throw 'Encoder interrupt capture not found' }
$menuSettings = [regex]::Match($main, '(?s)bool encoder_settings_ready.*?(?=\r?\n#endif)').Value
if (!$menuSettings) { throw 'Menu settings persistence functions not found' }
$menuStorage = Get-Content (Join-Path $root 'main\encoder_settings.cpp') -Raw
$menuStorage = [regex]::Replace($menuStorage, '(?m)^#include .*\r?$', '')
$menuStorage = $menuStorage.Replace('extern uint8_t _FS_start, _FS_end;', '')
$menuStorage = $menuStorage.Replace('&_FS_start', 'flash.data()').Replace('&_FS_end', '(flash.data()+flash.size())')
$encoderTable = [regex]::Match($main, '(?s)#ifdef OPTION_ENCODER_HALF_STEP_MODE.*?(?=\r?\n#endif //FEATURE_ENCODER_SUPPORT)').Value
$settings = Get-Content (Join-Path $root 'main\keyer_settings_pico.h') -Raw
$encoderDirection = [regex]::Match($settings, '(?s)#ifndef ENCODER_VFO_DIRECTION.*?(?=\r?\n#define sd_card_spi_ss_line)').Value
if (!$encoderDirection) { throw 'Encoder VFO direction setting not found' }
$encoderFunctions = @('read_audio_safe_touch', 'lcd_timed_message_expired', 'gps_updates_paused', 'update_gps_clock_icon', 'service_host_ptt_feedback', 'service_keyer_midi', 'midi_pod_mode', 'sync_pod_volume', 'audio_volume', 'audio_muted', 'step_audio_volume', 'apply_audio_levels', 'set_audio_volume', 'print_audio_volumes', 'cli_audio_volume',
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
#include <map>
#include "cw_rx_decoder.h"
#include "keyer_diagnostics.h"
#include "keyer_gps.h"
#include "encoder_menu.h"
#include "encoder_settings.h"
#define ARDUINO_ARCH_RP2040
#define FEATURE_WM8960_USB_AUDIO
#ifndef WM8960_MIC_GATE_MODE
#define WM8960_MIC_GATE_MODE 3
#endif
#ifndef WM8960_PTT_FEEDBACK_CHANNEL
#define WM8960_PTT_FEEDBACK_CHANNEL 0
#endif
#ifndef WM8960_PTT_FEEDBACK_TYPE
#define WM8960_PTT_FEEDBACK_TYPE 11
#endif
#ifndef WM8960_PTT_FEEDBACK_NUMBER
#define WM8960_PTT_FEEDBACK_NUMBER 18
#endif
#ifndef WM8960_MIC_LEVEL_FEEDBACK_CHANNEL
#define WM8960_MIC_LEVEL_FEEDBACK_CHANNEL 0
#endif
#ifndef WM8960_MIC_LEVEL_FEEDBACK_CC
#define WM8960_MIC_LEVEL_FEEDBACK_CC 23
#endif
#define FEATURE_INTERNET_LINK_MAX_LINKS 2
#define F(text) text
#define PRIMARY_SERIAL_CLS HardwareSerial
#define FEATURE_DISPLAY
#define FEATURE_LCD_BACKLIGHT_AUTO_DIM
uint32_t last_active_time = 0;
int backlight_on_calls = 0;
void tft_backlight(int state) { assert(state == 1); ++backlight_on_calls; }
using byte = uint8_t;
#define LCD_ROWS 3
#define LCD_COLUMNS 18
#ifndef WM8960_RX_TEXT_COLOR
#define WM8960_RX_TEXT_COLOR 0x07FF
#endif
#define FEATURE_TFT_DISPLAY
#define FEATURE_BT_KEYBOARD
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
#define NUM_BANK0_GPIOS 48
#define PICO_COMMAND_BUTTON_GPIO 14
#define WM8960_I2C_SDA_PIN 20
#define WM8960_I2C_SCL_PIN 21
#define WM8960_I2S_BCLK_PIN 18
#define WM8960_I2S_LRCLK_PIN 19
#define WM8960_I2S_DOUT_PIN 6
#define WM8960_I2S_DIN_PIN 7
#define WM8960_USE_PLL 1
#define WM8960_OUTPUT_ROUTE 2
#define WM8960_RX_CHANNEL 0
#define ENCODER_MENU_SELECTION_PULSES 2
#define TFT_CS 9
#define TFT_DC 8
#define TFT_RST 15
#define TFT_BL 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_SCLK 10
#define TOUCH_CS 16
#define tx_key_line_1 0
#define ptt_tx_1 0
#define tx_key_line_2 0
#define ptt_tx_2 0
#define tx_key_line_3 0
#define ptt_tx_3 0
#define tx_key_line_4 0
#define ptt_tx_4 0
#define tx_key_line_5 0
#define ptt_tx_5 0
#define tx_key_line_6 0
#define ptt_tx_6 0
#define sidetone_line 0
#define bt_keyboard_LED 0
constexpr int pico_encoder_pins[4][3] = {
  {rotary_pin1, rotary_pin2, rotary_switch ? rotary_switch : -1},
  {26,27,28}, {32,33,34}, {35,36,37}
};
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
  bool startsWith(const char *value) const { return text.rfind(value, 0) == 0; }
  char operator[](size_t index) const { return text[index]; }
  char charAt(size_t index) const { return index < text.size() ? text[index] : 0; }
  void concat(char character) { text += character; }
  long toInt() const { return std::strtol(text.c_str(), nullptr, 10); }
  friend String operator+(const String &left, const String &right) {
    return String(left.text + right.text);
  }
  const std::string &str() const { return text; }
};
uint8_t config_dirty = 0;
unsigned long config_changed_ms = 0;
void sync_pod_volume();
template<class T> T constrain(T value, int lo, int hi) {
  return std::max<T>(static_cast<T>(lo), std::min<T>(static_cast<T>(hi), value));
}
uint32_t clock_ms = 100, clock_us = 100000;
uint32_t millis() { return clock_ms; }
uint32_t micros() { return clock_us; }
void trace_audio_stage(uint8_t) {}
void advance(uint32_t ms) { clock_ms += ms; clock_us += ms * 1000; }
int touch_audio_services = 0;
void myDelay(uint32_t ms) {
  assert(ms == 0); ++touch_audio_services; advance(1);
}
int core_num = 0;
int get_core_num() { return core_num; }
uint32_t save_and_disable_interrupts() { return 0; }
void restore_interrupts(uint32_t) {}
struct spin_lock_t { int unused; };
spin_lock_t *render_spin_lock = nullptr;
uint32_t spin_lock_blocking(spin_lock_t *) { return 0; }
void spin_unlock(spin_lock_t *, uint32_t) {}
bool mounted = true, suspended = false, writable = true, lockable = true;
int write_budget = -1;
struct { int mutex; } USB;
struct CoreMutex {
  bool acquired;
  CoreMutex(int *, bool = true) : acquired(lockable) {}
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
  size_t write(uint8_t value) { output += static_cast<char>(value); return 1; }
  void print(const char *text) { output += text; }
  void println(const char *text) { output += text; output += '\n'; }
  void println() { output += '\n'; }
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
bool initialization_attempted = true, codec_write_ok = true;
std::vector<std::array<uint16_t,2>> codec_writes;
bool write_codec_register(uint8_t reg, uint16_t value) {
  codec_writes.push_back({reg,value}); return codec_write_ok;
}
uint8_t mic_gate_mode = WM8960_MIC_GATE_MODE;
uint32_t rx_text_dropped = 0;
volatile bool tone_active = false, tone_mute_pending = false;
volatile bool midi_tone_active = false, midi_ptt_active = false;
volatile bool local_voice_ptt = false, external_voice_ptt = false;
volatile uint32_t tone_stopped_ms = 0, tone_mute_release_ms = 0;
uint32_t tone_phase = 0, tone_phase_increment = 0, tone_frames_remaining = 0;
uint32_t midi_tone_phase = 0, midi_tone_phase_increment = 0, tone_output_frames = 0;
constexpr uint16_t kToneEdgeFrames = kSampleRate * 3 / 1000;
uint16_t tone_edge_level = 0;
uint32_t tone_out_phase = 0, tone_out_increment = 0;
uint32_t playback_read_index = 0, playback_write_index = 0;
uint32_t playback_starved_frames = 0;
uint32_t playback_min_queued = 1024;
uint32_t playback_frames_out = 0;
bool playback_started = false;
uint16_t playback_mute_level = kMuteRampFrames;
int32_t local_sidetone_gain_q15 = 32767, local_playback_gain_q15 = 32767;
int32_t local_microphone_gain_q15 = 32767;
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
enum class AudioVolumeTarget : uint8_t { Playback, Sidetone, Microphone, LocalPlayback };
int audio_volume(AudioVolumeTarget);
void service_keyer_midi();
void service_usb_cw_decoder();
void tft_fill_rect_cooperative(int,int,int,int,uint16_t);
int step_audio_volume(int, bool);
void set_audio_volume(AudioVolumeTarget, int, bool);
bool audio_muted(AudioVolumeTarget);
bool midi_pod_mode();
uint8_t pod_play_volume = 100;
bool pod_play_muted = false;
uint8_t pod_mic_volume = 100;
bool pod_mic_muted = false;
template<class T> T max(T a, T b) { return std::max(a,b); }
template<class T> T min(T a, T b) { return std::min(a,b); }
bool apply_audio_levels();
constexpr int LOW = 0, HIGH = 1, INPUT = 0, INPUT_PULLUP = 2;
constexpr int KEYER_COMMAND_MODE = 1;
#define FEATURE_STRAIGHT_KEY
constexpr int paddle_left = 30, paddle_right = 31, pin_straight_key = 38;
bool paddle_left_state = true, paddle_right_state = true, straight_key_state = true;
int keyer_machine_mode = 0;
int pin_values[48] = {};
constexpr int CHANGE = 3;
std::vector<int> interrupt_pins;
void (*encoder_irq)() = nullptr;
void (*encoder_irqs[48])() = {};
int digitalPinToInterrupt(int pin) { return pin; }
void attachInterrupt(int pin, void (*handler)(), int mode) {
  assert(mode == CHANGE); interrupt_pins.push_back(pin); encoder_irqs[pin] = handler;
  if (pin == rotary_pin1) encoder_irq = handler;
}
HardwareSerial *debug_serial_port = &terminal;
std::vector<std::array<int,2>> pin_modes;
int digitalRead(int pin) { assert(pin != 0); return pin_values[pin]; }
void pinMode(int pin, int mode) {
  assert(pin != 0); pin_modes.push_back({pin,mode});
}
void digitalWrite(int, int) { assert(false); }
std::string lcd_text;
constexpr uint8_t LCD_CLEAR = 0, LCD_TIMED_MESSAGE = 1;
uint8_t lcd_status = LCD_CLEAR;
uint32_t lcd_timed_message_clear_time = 0;
int display_expirations = 0;
void service_display() {
  ++display_expirations; lcd_status = LCD_CLEAR; lcd_text.clear();
}
constexpr int TFT_BLACK = 0, ICON_ANCHOR = 250, STATUS_BAR_FONT = 2;
constexpr int TFT_YELLOW = 1, TFT_GREY = 2, TIME_ANCHOR = 0, STATUS_BAR_Y_CURSOR = 6;
constexpr int TFT_WHITE = 3, TFT_BLUE = 4, TFT_DARKGREEN = 5, TFT_DARKGREY = 6;
constexpr int COLUMN_WIDTH = 10, SCROLL_TEXT_LEFT_SIDE = 4, GFXFF = 1;
constexpr int FONT_HEIGHT = 20;
constexpr int TL_DATUM = 0, SCROLL_BOX_DATUM = 1, SCROLL_BOX_FONT = 2;
constexpr int SCROLL_BOX_LEFT_SIDE = 2, SCROLL_BOX_TOP = 39, SCROLL_BOX_WIDTH = 476, SCROLL_BOX_HEIGHT = 138;
int in_pairing_mode = 0;
bool popup_active = false, encoder_menu_touch_capture = false;
bool lcd_scroll_buffer_dirty = false;
int menu_clears = 0;
void lcd_scroll_box_clear() { ++menu_clears; }
char time_str[27] = {};
bool stop_msg = false;
bool gps_time_active = true;
volatile uint8_t key_state = 0, ptt_line_activated = 0;
uint8_t manual_ptt_invoke = 0, manual_ptt_invoke_ptt_input_pin = 0;
int ptt_key_calls = 0, ptt_unkey_calls = 0;
void ptt_key() {
  ++ptt_key_calls;
  if (!ptt_line_activated) { ptt_line_activated = 1; wm8960_midi_ptt(true); }
}
void ptt_unkey() {
  ++ptt_unkey_calls;
  if (ptt_line_activated) { ptt_line_activated = 0; wm8960_midi_ptt(false); }
}
struct MockLcd {
  uint16_t pressure = 0, raw_x = 100, raw_y = 150;
  bool unstable = false;
  int raw_reads = 0;
  uint16_t getTouchRawZ() { return pressure; }
  void getTouchRaw(uint16_t *x, uint16_t *y) {
    *x = raw_x + (unstable && (++raw_reads % 2 == 0) ? 30 : 0); *y = raw_y;
  }
  void convertRawXY(uint16_t *, uint16_t *) {}
  int width() { return 480; }
  int height() { return 320; }
  int draws = 0;
  std::string label;
  std::vector<std::string> menu_value_text;
  int scroll_color = TFT_WHITE;
  std::vector<std::array<int,2>> scroll_glyphs;
  std::vector<std::array<int,4>> scroll_positions;
  uint32_t scroll_draw_us = 0;
  bool simulate_menu_latency = false;
  void setTextColor(int color) { scroll_color = color; }
  void drawChar(char c,int x,int y,int) {
    scroll_glyphs.push_back({c,scroll_color});
    scroll_positions.push_back({c,x,y,scroll_color});
    clock_us += scroll_draw_us;
  }
  std::vector<std::array<int,5>> rectangles;
  void fillRect(int x,int y,int w,int h,int color) {
    assert(x >= SCROLL_BOX_LEFT_SIDE && y >= SCROLL_BOX_TOP &&
        x+w <= SCROLL_BOX_LEFT_SIDE+SCROLL_BOX_WIDTH && y+h <= SCROLL_BOX_TOP+SCROLL_BOX_HEIGHT);
    rectangles.push_back({x,y,w,h,color});
    clock_us += scroll_draw_us;
    if (simulate_menu_latency) clock_us += w*h*4;
  }
  void fillRoundRect(int x,int y,int w,int h,int,int color) { fillRect(x,y,w,h,color); }
  void setTextDatum(int) {}
  void setTextPadding(int) {}
  bool free_font_active = true;
  int text_size = 2;
  void setFreeFont(int) { free_font_active = true; }
  void setTextFont(int font) { assert(font == 1); free_font_active = false; }
  void setTextSize(int size) { assert(size == 1 || size == 2); text_size = size; }
  uint16_t color565(int r, int g, int b) {
    assert(r == 144 && g == 238 && b == 144); return 0x9772;
  }
  void setTextColor(int fg, int bg) {
    assert(((fg == 0x9772 || fg == TFT_YELLOW || fg == TFT_GREY || fg == TFT_BLACK) && bg == TFT_BLACK) ||
        (fg == TFT_WHITE && (bg == TFT_BLACK || bg == TFT_BLUE || bg == TFT_DARKGREEN || bg == TFT_DARKGREY)));
  }
  void drawString(const char *text, int x, int row, int font) {
    assert(((x == ICON_ANCHOR || x == TIME_ANCHOR) && row == 6 && font == STATUS_BAR_FONT) ||
        (x >= SCROLL_BOX_LEFT_SIDE && x < SCROLL_BOX_LEFT_SIDE+SCROLL_BOX_WIDTH && row >= SCROLL_BOX_TOP &&
         row < SCROLL_BOX_TOP+SCROLL_BOX_HEIGHT && (font == 1 || font == 2)));
    label = text; ++draws;
    if (simulate_menu_latency) clock_us += 10000;
    if (font == 1) {
      assert(!free_font_active && (text_size == 1 || text_size == 2));
      assert(x + strlen(text)*6*text_size <= SCROLL_BOX_LEFT_SIDE+SCROLL_BOX_WIDTH);
      menu_value_text.push_back(text);
    }
  }
  void drawString(String text,int x,int y,int font) { drawString(text.str().c_str(),x,y,font); }
} lcd;
HardwareSerial &Wm8960Terminal = terminal;
int convert_cw_number_to_ascii(long pattern) { assert(pattern == 111); return 'S'; }
String lcd_scroll_buffer[LCD_ROWS];
uint16_t lcd_scroll_colors[LCD_ROWS][LCD_COLUMNS]{};
uint16_t lcd_scroll_holding_color = TFT_WHITE;
byte lcd_scroll_column_pointer = 0, lcd_scroll_row_pointer = 0, lcd_scroll_holding_space = 0;
byte lcd_scroll_flag = 0, lcd_scroll_drawn_row = 0, lcd_scroll_drawn_col = 0, lcd_scroll_repaint_row = 0;
byte lcd_scroll_repaint_col = 255;
bool lcd_scroll_needs_clear = false;
constexpr int LCD_SCROLL_MSG = 4;
int32_t tft_scroll_row_y(byte row) { return 60+row*20; }
void display_scroll_print_char_color(char,uint16_t);
void lcd_center_print_timed(String text, int, int duration) {
  assert(duration == 1000); lcd_text = text.str();
}
void speed_change(int step) { configuration.wpm += step; config_dirty = 1; }
void speed_set(int wpm) { configuration.wpm = wpm; config_dirty = 1; }
void speed_change_command_mode(int step) {
  configuration.wpm_command_mode += step; config_dirty = 1;
}
'@

$menuStorageMocks = @'
unsigned send_buffer_bytes = 0;
bool dit_buffer = false, dah_buffer = false, async_eeprom_write = false;
int paddle_pin_read(int pin) { return pin == paddle_left ? paddle_left_state : paddle_right_state; }
std::array<uint8_t,65536> flash{};
unsigned flash_pauses = 0, flash_resumes = 0;
bool audio_flash_paused = false;
void wm8960_audio_pause_for_flash() { assert(!audio_flash_paused); audio_flash_paused = true; ++flash_pauses; }
void wm8960_audio_resume_after_flash() { assert(audio_flash_paused); audio_flash_paused = false; ++flash_resumes; }
struct LittleFSConfig {
  bool auto_format;
  explicit LittleFSConfig(bool value) : auto_format(value) {}
};
struct MockFileState {
  std::vector<uint8_t> data;
};
std::map<std::string,MockFileState> filesystem;
bool fs_write_ok = true, fs_rename_ok = true, fs_close_ok = true;
class File {
  MockFileState *state = nullptr;
  size_t position = 0;
  bool writing = false;
 public:
  File() = default;
  File(MockFileState *s, bool w) : state(s), writing(w) {}
  explicit operator bool() const { return state != nullptr; }
  size_t size() const { return state->data.size(); }
  size_t write(const uint8_t *data, size_t size) {
    assert(audio_flash_paused && writing);
    if (!fs_write_ok) return 0;
    state->data.assign(data, data+size);
    return size;
  }
  int read(uint8_t *out, size_t size) {
    const size_t count = std::min(size,state->data.size()-position);
    std::copy_n(state->data.data()+position,count,out);
    position += count;
    return static_cast<int>(count);
  }
  void flush() { assert(audio_flash_paused); }
  void close() {
    if (writing && !fs_close_ok) state->data.clear();
    state = nullptr;
  }
};
struct MockLittleFS {
  bool mount_ok = false;
  unsigned formats = 0;
  bool setConfig(LittleFSConfig config) { assert(!config.auto_format); return true; }
  bool begin() { assert(audio_flash_paused); return mount_ok; }
  bool format() { assert(audio_flash_paused); ++formats; mount_ok = true; return true; }
  bool exists(const char *path) { return filesystem.count(path) != 0; }
  File open(const char *path, const char *mode) {
    if (*mode == 'w') {
      filesystem[path].data.clear();
      return File(&filesystem[path],true);
    }
    const auto item = filesystem.find(path);
    return item == filesystem.end() ? File() : File(&item->second,false);
  }
  bool rename(const char *from, const char *to) {
    assert(audio_flash_paused);
    if (!fs_rename_ok) return false;
    filesystem[to] = filesystem.at(from);
    filesystem.erase(from);
    return true;
  }
} LittleFS;
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
  control_mode = Wm8960MidiControlMode::Report;
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
  for (int width : {318,476})
    for (int count=1; count<=11; ++count)
      for (int selected=0; selected<count; ++selected) {
        const EncoderMenuLayout strip = {2,39,width,138,count,selected};
        for (int index=0; index<count; ++index) {
          if (!strip.visible(index)) continue;
          assert(strip.cell_x(index) >= strip.x);
          assert(strip.cell_x(index)+strip.box_width(index) <= strip.x+strip.width);
          assert(strip.hit(strip.cell_x(index)+5,strip.cell_y(index)+5) == index);
          if (index != selected) assert(strip.box_width(index) < strip.center_width());
        }
      }
  fresh();
#ifndef TEST_CW_ENCODERS_ONLY
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
  fresh(); tone_edge_level = 0; wm8960_audio_set_midi_input(true,false,750);
  render_i2s_output();
  assert(std::any_of(wm8960_i2s::output.begin(),wm8960_i2s::output.end(),
                    [](int16_t sample) { return sample != 0; }));
  assert(std::abs(wm8960_i2s::output[0]) < 64 && std::abs(wm8960_i2s::output[2]) < 64);
  render_i2s_output(); assert(tone_edge_level == kToneEdgeFrames);
  wm8960_audio_set_midi_input(false,false,750); render_i2s_output();
  assert(std::any_of(wm8960_i2s::output.begin(),wm8960_i2s::output.begin()+16,
                    [](int16_t sample) { return sample != 0; }));
  assert(tone_edge_level == kToneEdgeFrames - 128);
  render_i2s_output(); assert(tone_edge_level == 0);
  assert(std::all_of(wm8960_i2s::output.begin()+2*(kToneEdgeFrames-128),
                    wm8960_i2s::output.end(),
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
  assert(terminal.output.find("\\:midi-ptt\nMIDI PTT ") == 0);
  terminal.output = "\\:midi";
  assert(cli_midi(&terminal, "MIDI"));
  assert(terminal.output.find("\\:midi\nMIDI PTT ") == 0);
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
  // LOCAL mode: no control reports, but keys and the VFO wheel still go out.
  fresh(); assert(wm8960_midi_set_control_mode(0));
  assert(!wm8960_midi_set_control_mode(3));
  assert(wm8960_midi_control_mode() == Wm8960MidiControlMode::Local);
  mounted = false; service(); outgoing.clear();
  mounted = true; service();
  assert(outgoing.size() == 1 && messages(0x99,17,0) == 1);
  outgoing.clear(); wm8960_midi_service(900, 35, 10, 20, 30);
  advance(300000); wm8960_midi_service(900, 35, 10, 20, 30);
  assert(outgoing.empty());
  assert(wm8960_midi_wheel_step(1)); service(); assert(messages(0xB9,20,127) == 1);
  // Leaving LOCAL resends every current value once.
  outgoing.clear(); assert(wm8960_midi_set_control_mode(1));
  wm8960_midi_service(1000, 20, 100, 0, 100);
  assert(outgoing.size() == 5 && messages(0xB9,21,41) == 1 && messages(0xB9,3,127) == 1);
  assert(messages(0xB9,7,127) == 1 && messages(0xB9,22,0) == 1 && messages(0xB9,23,127) == 1);
  // POD mode: master volume goes only to the PC; other levels stay local.
  fresh();
  configuration.midi_control_mode = 1; configuration.usb_play_volume = 70;
  configuration.usb_play_muted = false; configuration.usb_mic_volume = 60;
  configuration.usb_mic_muted = false; config_dirty = 0;
  terminal.output.clear();
  assert(cli_midi(&terminal, "MIDI-MODE POD"));
  assert(configuration.midi_control_mode == 2 && config_dirty == 1);
  assert(terminal.output.find("MODE POD") != std::string::npos);
  assert(midi_pod_mode() && audio_volume(AudioVolumeTarget::Playback) == 70);
  set_audio_volume(AudioVolumeTarget::Playback, 40, false);
  assert(configuration.usb_play_volume == 70 && pod_play_volume == 40);
  set_audio_volume(AudioVolumeTarget::Sidetone, 30, false);
  assert(configuration.sidetone_volume == 30);
  outgoing.clear(); service_keyer_midi();
  assert(messages(0xB9,7,51) == 1);
  set_audio_volume(AudioVolumeTarget::Playback, audio_volume(AudioVolumeTarget::Playback),
      !audio_muted(AudioVolumeTarget::Playback));
  assert(!configuration.usb_play_muted && audio_muted(AudioVolumeTarget::Playback));
  outgoing.clear(); service_keyer_midi(); assert(messages(0xB9,7,0) == 1);
  // POD mode: the USB mic level is also PC-only.
  set_audio_volume(AudioVolumeTarget::Microphone, 20, false);
  assert(configuration.usb_mic_volume == 60 && pod_mic_volume == 20);
  assert(audio_volume(AudioVolumeTarget::Microphone) == 20);
  outgoing.clear(); service_keyer_midi(); assert(messages(0xB9,23,25) == 1);
  // PC-side changes are mirrored without touching local levels or replying.
  auto pod_cc = [](uint8_t status, uint8_t control, uint8_t value) {
    advance(20); incoming.push_back({11,status,control,value}); service_keyer_midi();
  };
  auto sent_control = [](uint8_t control) {
    return std::any_of(outgoing.begin(), outgoing.end(), [control](const auto &p) {
      return p[1] == 0xB9 && p[2] == control;
    });
  };
  config_dirty = 0;
  outgoing.clear(); pod_cc(0xB9, 7, 100); service_keyer_midi();
  assert(pod_play_volume == 79 && !pod_play_muted && !sent_control(7));
  assert(configuration.usb_play_volume == 70 && config_dirty == 0);
  outgoing.clear(); pod_cc(0xB9, 23, 120); service_keyer_midi();
  assert(pod_mic_volume == 94 && !pod_mic_muted && !sent_control(23));
  assert(configuration.usb_mic_volume == 60 && config_dirty == 0);
  outgoing.clear(); pod_cc(0xB8, 7, 10);
  assert(pod_play_volume == 79); // Other channels are not mirrored.
  // PC speed, sidetone level and frequency update the local keyer without a reply.
  configuration.wpm = 20; configuration.hz_sidetone = 750;
  configuration.sidetone_volume = 30; configuration.sidetone_muted = true;
  service_keyer_midi(); config_dirty = 0;
  outgoing.clear(); pod_cc(0xB9, 21, 62); service_keyer_midi();
  assert(configuration.wpm == 30 && config_dirty && !sent_control(21));
  config_dirty = 0;
  outgoing.clear(); pod_cc(0xB9, 22, 32); service_keyer_midi();
  assert(configuration.sidetone_volume == 25 && !configuration.sidetone_muted);
  assert(config_dirty && !sent_control(22));
  config_dirty = 0;
  outgoing.clear(); pod_cc(0xB9, 3, 64); service_keyer_midi();
  assert(configuration.hz_sidetone == 653 && config_dirty && !sent_control(3));
  outgoing.clear(); pod_cc(0xB9, 21, 0); service_keyer_midi();
  assert(configuration.wpm == 30); // Speed value zero is ignored, as in Thetis.
  for (uint32_t wpm = 1; wpm <= 60; ++wpm) {
    host_control_value[static_cast<uint8_t>(Wm8960MidiHostControl::Speed)] =
        speed_controller_value(wpm);
    uint16_t decoded = 0;
    assert(wm8960_midi_take_host_control(Wm8960MidiHostControl::Speed, &decoded) &&
           decoded == wpm);
  }
  for (uint16_t hz = 300; hz <= 1000; ++hz) {
    host_control_value[static_cast<uint8_t>(Wm8960MidiHostControl::SidetoneFrequency)] =
        ((hz - 300u) * 127u + 350u) / 700u;
    uint16_t decoded = 0;
    assert(wm8960_midi_take_host_control(Wm8960MidiHostControl::SidetoneFrequency, &decoded));
    assert(decoded + 3 >= hz && decoded <= hz + 3);
  }
  last_control_inputs[3] = configuration.hz_sidetone;
  // Local changes after PC sync are still reported.
  configuration.hz_sidetone = 1000; outgoing.clear(); service_keyer_midi();
  assert(messages(0xB9,3,127) == 1);
  // The local encoder continues from the mirrored PC level.
  set_audio_volume(AudioVolumeTarget::Playback,
      step_audio_volume(audio_volume(AudioVolumeTarget::Playback), false), false);
  outgoing.clear(); service_keyer_midi();
  assert(pod_play_volume == 70 && messages(0xB9,7,89) == 1);
  saved = configuration; config_dirty = 0;
  assert(cli_midi(&terminal, "MIDI-MODE BOGUS"));
  assert(memcmp(&saved, &configuration, sizeof(saved)) == 0 && !config_dirty);
  assert(cli_midi(&terminal, "MIDI-MODE REPORT"));
  assert(configuration.midi_control_mode == 1 && !midi_pod_mode());
  assert(audio_volume(AudioVolumeTarget::Playback) == 70 &&
         !audio_muted(AudioVolumeTarget::Playback));
  outgoing.clear(); service_keyer_midi(); assert(messages(0xB9,7,89) == 1);
  pod_cc(0xB9, 7, 10);
  assert(configuration.usb_play_volume == 70); // REPORT ignores PC volume.
  pod_cc(0xB9, 21, 100); service_keyer_midi();
  assert(configuration.wpm == 30); // REPORT ignores PC speed.
  assert(cli_midi(&terminal, "MIDI-MODE LOCAL"));
  assert(configuration.midi_control_mode == 0);
  outgoing.clear(); service_keyer_midi(); assert(outgoing.empty());
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
  tone_active = midi_tone_active = midi_ptt_active = tone_mute_pending = false;
  for (bool local : {false,true}) {
    for (bool external : {false,true}) {
      wm8960_audio_set_local_ptt(local);
      wm8960_audio_set_external_ptt(external);
      const bool allowed = WM8960_MIC_GATE_MODE == 0 ||
          ((WM8960_MIC_GATE_MODE == 1 || WM8960_MIC_GATE_MODE == 3) && local) ||
          ((WM8960_MIC_GATE_MODE == 2 || WM8960_MIC_GATE_MODE == 3) && external);
      assert(microphone_muted_for_gate() == !allowed);
      tone_active = true; assert(microphone_muted_for_gate()); tone_active = false;
      midi_tone_active = true; assert(microphone_muted_for_gate()); midi_tone_active = false;
    }
  }
  wm8960_audio_set_local_ptt(false); wm8960_audio_set_external_ptt(false);
  wm8960_audio_set_external_ptt(true); tud_umount_cb();
  assert(!external_voice_ptt);
  manual_ptt_invoke = 0; ptt_line_activated = 1;
  service_keyer_midi(); assert(!local_voice_ptt);
  manual_ptt_invoke = 1;
  service_keyer_midi(); assert(local_voice_ptt);
  ptt_line_activated = 0;
  service_keyer_midi(); assert(!local_voice_ptt);
  manual_ptt_invoke = 0;
  wm8960_audio_set_local_ptt(true);
  tone_mute_pending = true; tone_stopped_ms = clock_ms; tone_mute_release_ms = 420;
  assert(microphone_muted_for_gate());
  tone_mute_pending = false;
  wm8960_audio_set_local_ptt(false);
  fresh();
  const uint8_t feedback_cin = WM8960_PTT_FEEDBACK_TYPE;
  const uint8_t feedback_status = static_cast<uint8_t>((feedback_cin << 4) | 9);
  const size_t feedback_queue_count = queue_count;
  receive(feedback_status,WM8960_PTT_FEEDBACK_NUMBER,127,feedback_cin);
  assert(external_voice_ptt == (WM8960_PTT_FEEDBACK_CHANNEL == 10));
  if (WM8960_PTT_FEEDBACK_CHANNEL == 10) assert(queue_count == feedback_queue_count);
  receive(feedback_status,WM8960_PTT_FEEDBACK_NUMBER,0,feedback_cin);
  assert(!external_voice_ptt);
  if (WM8960_PTT_FEEDBACK_TYPE == 9) {
    receive(feedback_status,WM8960_PTT_FEEDBACK_NUMBER,127,9);
    receive(0x89,WM8960_PTT_FEEDBACK_NUMBER,64,8);
    assert(!external_voice_ptt);
  }
  receive(feedback_status - 1,WM8960_PTT_FEEDBACK_NUMBER,127,feedback_cin);
  assert(!external_voice_ptt);
  if (WM8960_PTT_FEEDBACK_CHANNEL == 10 && WM8960_PTT_FEEDBACK_TYPE == 9 &&
      WM8960_PTT_FEEDBACK_NUMBER == 18) {
    // SDRoxide feedback is byte-identical to the keyer's own PTT message.
    fresh(); assert(wm8960_midi_set_ptt_mode(1));
    wm8960_midi_ptt(true); service();
    incoming.push_back({9,0x99,18,127}); service();
    assert(external_voice_ptt);
    wm8960_midi_ptt(false); service();
    incoming.push_back({9,0x99,18,0}); service();
    assert(!external_voice_ptt);
  }
  // Host PTT feedback drives the keyer PTT output without sending MIDI back.
  fresh(); assert(wm8960_midi_set_ptt_mode(2));
  manual_ptt_invoke = 0; ptt_line_activated = 0; key_state = 0;
  wm8960_audio_set_external_ptt(true); service_host_ptt_feedback(); service();
  assert(ptt_line_activated && manual_ptt_invoke && messages(0x99,18,127) == 0);
  wm8960_audio_set_external_ptt(false); service_host_ptt_feedback(); service();
  assert(!ptt_line_activated && !manual_ptt_invoke && messages(0x99,18,127) == 0);
  // A local release of host-started PTT still tells the host.
  wm8960_audio_set_external_ptt(true); service_host_ptt_feedback(); service();
  manual_ptt_invoke = 0; ptt_unkey(); service();
  assert(!ptt_line_activated && messages(0x99,18,127) == 1);
  wm8960_audio_set_external_ptt(false); service_host_ptt_feedback(); service();
  assert(messages(0x99,18,127) == 1);
  // Host turns off local manual PTT: release without toggling the host back on.
  fresh(); assert(wm8960_midi_set_ptt_mode(2));
  manual_ptt_invoke = 1; ptt_key(); service();
  assert(messages(0x99,18,127) == 1);
  wm8960_audio_set_external_ptt(true); service_host_ptt_feedback(); service();
  wm8960_audio_set_external_ptt(false); service_host_ptt_feedback(); service();
  assert(!ptt_line_activated && !manual_ptt_invoke && messages(0x99,18,127) == 1);
  // Automatic PTT is never converted into a held manual PTT.
  fresh(); assert(wm8960_midi_set_ptt_mode(2));
  manual_ptt_invoke = 0; ptt_key(); service();
  wm8960_audio_set_external_ptt(true); service_host_ptt_feedback();
  assert(!manual_ptt_invoke && ptt_line_activated);
  wm8960_audio_set_external_ptt(false); service_host_ptt_feedback();
  assert(ptt_line_activated);
  ptt_unkey(); service();
  assert(messages(0x99,18,127) == 1);
  // Host off while keyed: hand PTT back to the tail timer.
  fresh(); assert(wm8960_midi_set_ptt_mode(2));
  wm8960_audio_set_external_ptt(true); service_host_ptt_feedback();
  key_state = 1;
  const int unkeys_before = ptt_unkey_calls;
  wm8960_audio_set_external_ptt(false); service_host_ptt_feedback();
  assert(ptt_line_activated && !manual_ptt_invoke && ptt_unkey_calls == unkeys_before);
  key_state = 0; ptt_unkey(); service();
  assert(messages(0x99,18,127) == 0);
  manual_ptt_invoke = 0; ptt_line_activated = 0;
  // Host mic-level feedback sets the saved keyer mic level without echoing it back.
  fresh();
  auto host_cc = [](uint8_t status, uint8_t control, uint8_t value) {
    advance(20); incoming.push_back({11,status,control,value});
    service_keyer_midi(); service_keyer_midi();
  };
  configuration.usb_mic_volume = 40; configuration.usb_mic_muted = true;
  service_keyer_midi(); outgoing.clear(); config_dirty = 0;
  const size_t mic_queue_before = queue_count;
  host_cc(0xB9,23,95);
  if (WM8960_MIC_LEVEL_FEEDBACK_CHANNEL == 10) {
    assert(configuration.usb_mic_volume == 75 && !configuration.usb_mic_muted && config_dirty);
    assert(config_changed_ms == clock_ms);
    assert(outgoing.empty() && queue_count == mic_queue_before);
    // Every host value maps to a level that is not reported back.
    for (uint8_t value = 0; value <= 127; ++value) {
      advance(1100); outgoing.clear();
      host_cc(0xB9,23,value);
      assert(configuration.usb_mic_volume == (value * 100 + 63) / 127);
      assert(outgoing.empty());
    }
    // Feedback of the keyer's own recent report is ignored.
    advance(1100); configuration.usb_mic_volume = 50;
    service_keyer_midi(); assert(messages(0xB9,23,64) == 1);
    configuration.usb_mic_volume = 60; service_keyer_midi();
    host_cc(0xB9,23,64);
    assert(configuration.usb_mic_volume == 60);
    advance(1100); host_cc(0xB9,23,64);
    assert(configuration.usb_mic_volume == 50);
    // Other channels and controllers are not mic-level feedback.
    host_cc(0xB8,23,20); host_cc(0xB9,24,20);
    assert(configuration.usb_mic_volume == 50);
    host_mic_value = 10; reset_session(); service_keyer_midi();
    assert(configuration.usb_mic_volume == 50);
    // Core 1 must not apply host levels while core 0 reports them.
    host_mic_value = 25; core_num = 1; service_keyer_midi();
    assert(configuration.usb_mic_volume == 50 && host_mic_value == 25);
    core_num = 0; outgoing.clear(); service_keyer_midi();
    assert(configuration.usb_mic_volume == 20 && messages(0xB9,23,25) == 0);
  } else {
    assert(configuration.usb_mic_volume == 40 && configuration.usb_mic_muted);
  }
  configuration.usb_mic_volume = 100; configuration.usb_mic_muted = false;
  fresh();
#endif
  std::fill(std::begin(pin_values), std::end(pin_values), HIGH);
  uint16_t touch_x = 0, touch_y = 0;
  lcd.pressure = 0;
  assert(!read_audio_safe_touch(&touch_x, &touch_y, 320));
  lcd.pressure = 400;
  const int services_before_touch = touch_audio_services;
  assert(read_audio_safe_touch(&touch_x, &touch_y, 320));
  assert(touch_x == 100 && touch_y == 150 && touch_audio_services - services_before_touch >= 5);
  lcd.pressure = 100;
  assert(read_audio_safe_touch(&touch_x, &touch_y, 320));
  lcd.pressure = 0; assert(!read_audio_safe_touch(&touch_x, &touch_y, 320));
  lcd.pressure = 100; assert(!read_audio_safe_touch(&touch_x, &touch_y, 320));
  lcd.pressure = 400; lcd.unstable = true; lcd.raw_reads = 0;
  assert(!read_audio_safe_touch(&touch_x, &touch_y, 320));
  lcd.unstable = false; lcd.raw_x = 500;
  assert(!read_audio_safe_touch(&touch_x, &touch_y, 320));
  lcd.raw_x = 100; lcd.pressure = 0;
  lcd_status = LCD_TIMED_MESSAGE;
  lcd_timed_message_clear_time = clock_ms + 1000;
  assert(!lcd_timed_message_expired());
  advance(999); assert(!lcd_timed_message_expired());
  advance(1); assert(lcd_timed_message_expired());
  ptt_line_activated = 1; check_rotary_encoder();
  assert(display_expirations == 0);
  ptt_line_activated = 0; key_state = 1; check_rotary_encoder();
  assert(display_expirations == 0);
  key_state = 0; check_rotary_encoder();
  assert(display_expirations == 1 && lcd_status == LCD_CLEAR);
  clock_ms = UINT32_MAX - 1000;
  lcd_status = LCD_TIMED_MESSAGE; lcd_timed_message_clear_time = clock_ms + 1000;
  assert(!lcd_timed_message_expired());
  advance(1000); assert(lcd_timed_message_expired());
  lcd_status = LCD_CLEAR;
  configuration.ignore_gps = false; stop_msg = false;
  strcpy(time_str, "12:34:56");
  lcd.draws = 0; update_gps_clock_icon(); assert(lcd.draws == 1);
  for (int count = 0; count < 1000; ++count) update_gps_clock_icon();
  assert(lcd.draws == 1);
  strcpy(time_str, "12:34:57"); update_gps_clock_icon();
  assert(lcd.draws == 2 && lcd.label == "12:34:57");
  stop_msg = true; update_gps_clock_icon(); assert(lcd.draws == 2);
  gps_time_active = false; update_gps_clock_icon(); assert(lcd.draws == 3);
  update_gps_clock_icon(); assert(lcd.draws == 3);
  stop_msg = false; gps_time_active = true; update_gps_clock_icon(); assert(lcd.draws == 4);
  configuration.ignore_gps = true; update_gps_clock_icon(); assert(lcd.draws == 5);
  configuration.ignore_gps = false; update_gps_clock_icon(); assert(lcd.draws == 6);
  assert(!gps_data_fresh(false, 0));
  assert(gps_data_fresh(true, 0) && gps_data_fresh(true, 4999));
  assert(!gps_data_fresh(true, 5000) && !gps_data_fresh(true, UINT32_MAX));
  assert(!gps_location_held(false, 0));
  assert(gps_location_held(true, 5000) && gps_location_held(true, 59999));
  assert(!gps_location_held(true, 60000) && !gps_location_held(true, UINT32_MAX));
  tone_active = true;
  tone_frames_remaining = 100;
  assert(wm8960_audio_feedback_pending());
  tone_frames_remaining = 0;
  assert(!wm8960_audio_feedback_pending());
  assert(wm8960_audio_tone_active() && gps_updates_paused());
  tone_active = false; midi_tone_active = true;
  assert(wm8960_audio_tone_active() && gps_updates_paused());
  midi_tone_active = false;
  assert(!wm8960_audio_tone_active() && !gps_updates_paused());
  for (uint32_t frequency : {400u,1500u}) {
    tone_active = true; tone_frames_remaining = 9600;
    tone_phase = 0;
    tone_phase_increment = uint32_t((uint64_t(frequency) << 32) / kSampleRate);
    advance(300); // Wall-clock expiry alone must not end sample-based feedback.
    assert(wm8960_audio_feedback_pending());
    const uint32_t before_frames = tone_output_frames;
    for (int block = 0; block < 75; ++block) render_i2s_output();
    assert(!wm8960_audio_feedback_pending());
    assert(tone_output_frames - before_frames == 9600);
  }
  for (uint8_t ptt : {0,1}) {
    for (uint8_t key : {0,1}) {
      ptt_line_activated = ptt; key_state = key;
      assert(gps_updates_paused() == bool(ptt || key));
      if (ptt || key) {
        strcpy(time_str, "12:35:00");
        update_gps_clock_icon(); assert(lcd.draws == 6);
      }
    }
  }
  ptt_line_activated = key_state = 0;
  update_gps_clock_icon(); assert(lcd.draws == 7 && lcd.label == "12:35:00");
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
  assert(pin_modes.size() == (rotary_switch ? 12 : 11));
  assert(pin_modes[0][1] == INPUT_PULLUP && pin_modes[1][1] == INPUT_PULLUP);
  assert(interrupt_pins == std::vector<int>({rotary_pin1, rotary_pin2,26,27,32,33,35,36}));
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
    draw_encoder_mode(6, true); assert(lcd.draws == draws + 2);
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
    assert(!encoder_menu.open);
    click();
    assert(encoder_menu.open && !encoder_menu.adjusting && encoder_menu.count == 11);
    assert(encoder_menu.controls[1] == static_cast<uint8_t>(EncoderFunction::Master));
    assert(encoder_menu.controls[2] == static_cast<uint8_t>(EncoderFunction::LocalRx));
    assert(std::string(encoder_control_attributes[10].label) == "Local Volume");
    assert(encoder_function == EncoderFunction::Vfo);
    configuration.usb_play_volume = 50;
    encoder_menu_rotate(1,0);
    assert(encoder_function == EncoderFunction::Vfo); // First pulse only accumulates.
    encoder_menu_rotate(-1,0);
    assert(encoder_function == EncoderFunction::Vfo); // Opposite pulse cancels.
    encoder_menu_rotate(1,0);
    encoder_menu_rotate(1,0);
    assert(encoder_function == EncoderFunction::Master && configuration.usb_play_volume == 50);
    click();
    assert(encoder_menu.adjusting);
    encoder_menu_rotate(1,0);
    assert(configuration.usb_play_volume == 52);
    click();
    assert(!encoder_menu.adjusting);
    encoder_menu_rotate(-1,0);
    assert(encoder_function == EncoderFunction::Master);
    encoder_menu_rotate(-1,0);
    assert(encoder_function == EncoderFunction::Vfo);
    assert(encoder_menu.open && !encoder_menu.adjusting);
    for (int cycle=0; cycle<2; ++cycle) {
      for (int step=1; step<=11; ++step) {
        encoder_menu_rotate(-1,0);
        encoder_menu_rotate(-1,0);
        assert(encoder_menu.open && encoder_menu.selected == (11-step)%11);
        assert(encoder_function == encoder_menu_order[encoder_menu.selected]);
      }
      for (int step=1; step<=11; ++step) {
        encoder_menu_rotate(1,0);
        encoder_menu_rotate(1,0);
        assert(encoder_menu.open && encoder_menu.selected == step%11);
        assert(encoder_function == encoder_menu_order[encoder_menu.selected]);
      }
    }
    assert(configuration.usb_play_volume == 52);
    click(); // Explicit VFO confirmation still closes the menu.
    assert(!encoder_menu.open);
    pin_values[rotary_switch] = LOW; check_encoder_switch();
    advance(20); check_encoder_switch();
    advance(999); check_encoder_switch();
    assert(encoder_menu.open);
    advance(1); check_encoder_switch();
    assert(!encoder_menu.open);
    pin_values[rotary_switch] = HIGH; check_encoder_switch();
    advance(20); check_encoder_switch();
    assert(!encoder_menu.open);
    click();
    clock_ms = UINT32_MAX-500;
    encoder_menu.activity_ms = clock_ms;
    pin_values[rotary_switch] = LOW; check_encoder_switch();
    advance(20); check_encoder_switch(); advance(1000); check_encoder_switch();
    assert(!encoder_menu.open);
    pin_values[rotary_switch] = HIGH; check_encoder_switch();
    advance(20); check_encoder_switch();
  } else {
    assert(encoder_function == EncoderFunction::Master);
    check_encoder_switch(); assert(encoder_function == EncoderFunction::Master);
  }
  open_encoder_menu(0);
  configuration.usb_play_volume = 46;
  configuration.sidetone_volume = 32;
  configuration.usb_mic_volume = 68;
  configuration.hz_sidetone = 750;
  configuration.wpm = 23;
  assert(wm8960_audio_set_output_route(Wm8960OutputRoute::Both));
  assert(wm8960_audio_configure_rx(0,600,0,0));
  encoder_menu.selected = 0;
  invalidate_encoder_menu();
  lcd.menu_value_text.clear();
  for (int n=0; n<12; ++n) { advance(25); service_encoder_menu(); }
  const auto vfo_layout = encoder_menu_layout();
  assert(encoder_control_is_relative(EncoderFunction::Vfo));
  assert(!encoder_control_is_relative(EncoderFunction::Master));
  assert(std::none_of(lcd.rectangles.begin(),lcd.rectangles.end(),[&](const auto &r) {
    return r[0] == vfo_layout.cell_x(0)+4 &&
        r[1] == vfo_layout.cell_y(0)+vfo_layout.box_height(0)-5;
  }));
  for (const char *text : {"Relative","46%"})
    assert(std::find(lcd.menu_value_text.begin(),lcd.menu_value_text.end(),text) != lcd.menu_value_text.end());
  for (int index=0; index<11; ++index) {
    encoder_menu.selected = index;
    invalidate_encoder_menu();
    for (int n=0; n<5; ++n) { advance(25); service_encoder_menu(); }
  }
  for (const char *text : {"32%","68%","750 Hz","23 WPM","BOTH","0 Off","600 Hz","Auto"})
    assert(std::find(lcd.menu_value_text.begin(),lcd.menu_value_text.end(),text) != lcd.menu_value_text.end());
  encoder_menu.selected = 7;
  invalidate_encoder_menu();
  assert(wm8960_audio_set_output_route(Wm8960OutputRoute::Headphone));
  for (int n=0; n<12; ++n) { advance(25); service_encoder_menu(); }
  assert(std::find(lcd.menu_value_text.begin(),lcd.menu_value_text.end(),"HEADPHONE") != lcd.menu_value_text.end());
  assert(strlen("HEADPHONE")*12 <= encoder_menu_layout().center_width()-8);
  assert(encoder_control_value_text(EncoderFunction::RxThreshold,40).str() == "40");
  assert(encoder_control_value_text(EncoderFunction::RxSpeed,20).str() == "20 WPM");
  assert(std::any_of(lcd.rectangles.begin(),lcd.rectangles.end(),
      [](const auto &r) { return r[4] == TFT_BLUE; }));
  encoder_menu.selected = 0;
  encoder_controls[0].function = EncoderFunction::Vfo;
  invalidate_encoder_menu();
  const auto layout = encoder_menu_layout();
  for (int index=0; index<encoder_menu.count; ++index)
    if (layout.visible(index))
      assert(layout.hit(layout.cell_x(index)+5,layout.cell_y(index)+5) == index);
  configuration.usb_play_volume = 50;
  const int master_x = layout.cell_x(1)+5, master_y = layout.cell_y(1)+5;
  assert(encoder_menu_touch(true,master_x,master_y));
  assert(encoder_function == EncoderFunction::Master && encoder_menu.adjusting);
  assert(encoder_menu_touch(true,master_x+24,master_y));
  assert(configuration.usb_play_volume == 56);
  lcd.menu_value_text.clear();
  assert(encoder_menu_touch(false,master_x+24,master_y));
  for (int n=0; n<12; ++n) { advance(25); service_encoder_menu(); }
  assert(std::find(lcd.menu_value_text.begin(),lcd.menu_value_text.end(),"56%") != lcd.menu_value_text.end());
  assert(std::any_of(lcd.rectangles.begin(),lcd.rectangles.end(),
      [](const auto &r) { return r[4] == TFT_DARKGREEN; }));
  assert(encoder_menu_touch(true,layout.x+layout.width-10,layout.y+5));
  assert(!encoder_menu.open);
  assert(encoder_menu_touch(true,master_x,master_y)); // Close touch cannot fall through to underlying buttons.
  assert(encoder_menu_touch(false,master_x,master_y));
  assert(!encoder_menu_touch(false,master_x,master_y));
  open_encoder_menu(0);
  encoder_menu.activity_ms = UINT32_MAX-2000;
  clock_ms = encoder_menu.activity_ms;
  advance(4999); service_encoder_menu(); assert(encoder_menu.open);
  advance(1); service_encoder_menu(); assert(!encoder_menu.open && lcd_scroll_buffer_dirty);
  encoder_controls[0].selection_pulses = 3;
  open_encoder_menu(0);
  const uint8_t selection_before = encoder_menu.selected;
  encoder_menu_rotate(1,0); encoder_menu_rotate(1,0);
  assert(encoder_menu.selected == selection_before);
  encoder_menu_rotate(1,0);
  assert(encoder_menu.selected != selection_before);
  close_encoder_menu();
  encoder_controls[0].selection_pulses = 2;
  encoder_function = EncoderFunction::Master;
  for (int level = 0; level <= 100; ++level) {
    assert(step_audio_volume(level, true) ==
        std::min(100, level + 2));
    assert(step_audio_volume(level, false) ==
        std::max(0, level - 2));
  }
  configuration.usb_play_volume = 100; configuration.usb_play_muted = true;
  lcd_text = "unchanged";
  apply_encoder_step(-1);
  assert(configuration.usb_play_volume == 98 && !configuration.usb_play_muted);
  assert(lcd_text == "unchanged");
  encoder_function = EncoderFunction::Sidetone;
  configuration.sidetone_volume = 50; apply_encoder_step(1);
  assert(configuration.sidetone_volume == 52);
  encoder_function = EncoderFunction::Microphone;
  configuration.usb_mic_volume = 0; apply_encoder_step(-1);
  assert(configuration.usb_mic_volume == 0);
  apply_encoder_step(1); assert(configuration.usb_mic_volume == 2);
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
    lcd_text = "unchanged";
    check_rotary_encoder();
    if (steps_per_turn == 2) check_rotary_encoder();
    assert(encoder_event_head == encoder_event_tail);
    if (function == EncoderFunction::Speed) assert(configuration.wpm == 100 - 20 * steps_per_turn);
    if (function == EncoderFunction::Frequency) assert(configuration.hz_sidetone == 1000 - 200 * steps_per_turn);
    advance(24); check_rotary_encoder(); assert(lcd_text == "unchanged");
    advance(1); check_rotary_encoder(); assert(lcd_text == "unchanged");
    lcd_text = "no repeat"; advance(100); check_rotary_encoder();
    assert(lcd_text == "no repeat");
  }
  encoder_function = EncoderFunction::Master;
  configuration.usb_play_volume = 100;
  turn(false);
  const auto pending_tail = encoder_event_tail;
  lcd_text = "blocked";
  for (uint8_t ptt : {0,1}) {
    for (uint8_t key : {0,1}) {
      if (!ptt && !key) continue;
      ptt_line_activated = ptt; key_state = key;
      const int draws = lcd.draws;
      check_rotary_encoder(); show_encoder_function(); draw_encoder_mode(6, true);
      assert(encoder_event_tail == pending_tail && configuration.usb_play_volume == 100);
      assert(lcd_text == "blocked" && lcd.draws == draws);
    }
  }
  ptt_line_activated = key_state = 0;
  advance(25); check_rotary_encoder();
  assert(encoder_event_tail != pending_tail && configuration.usb_play_volume < 100);
  assert(lcd_text == "blocked");
  encoder_function = EncoderFunction::Vfo;
  mounted = false; service(); apply_encoder_step(1);
  assert(lcd_text == "MIDI unavailable");
  fresh();
  assert(validate_pico_encoder_pins());
  encoder_function = EncoderFunction::Master;
  open_encoder_menu(0);
  advance(4999);
  const auto before_movement = encoder_event_head;
  for (int value : {1,0,2,3}) {
    pins(value);
    if (encoder_event_head != before_movement) break;
  }
  assert(encoder_event_head != before_movement);
  service_encoder_menu();
  assert(encoder_menu.open);
  check_rotary_encoder();
  assert(encoder_menu.open && encoder_function == EncoderFunction::Master);
  advance(4999); service_encoder_menu(); assert(encoder_menu.open);
  advance(1); service_encoder_menu(); assert(!encoder_menu.open);
  encoder_function = EncoderFunction::Master;
  configuration.usb_play_volume = 100;
  open_encoder_menu(0);
  encoder_menu_switch(0);
  advance(4999); encoder_menu_rotate(1,0); // Adjustment at the limit is still activity.
  advance(4999); service_encoder_menu(); assert(encoder_menu.open);
  advance(1); service_encoder_menu(); assert(!encoder_menu.open);
  close_encoder_control_popup();
  encoder_function = EncoderFunction::Master;
  configuration.usb_play_volume = 50;
  lcd.rectangles.clear();
  lcd.menu_value_text.clear();
  const int clears_before_popup = menu_clears;
  apply_encoder_step(1);
  assert(encoder_control_popup.active);
  service_encoder_menu();
  const EncoderMenuLayout popup_layout = {SCROLL_BOX_LEFT_SIDE,SCROLL_BOX_TOP,
      SCROLL_BOX_WIDTH,SCROLL_BOX_HEIGHT,10};
  const int popup_width = popup_layout.center_width();
  const int popup_height = popup_layout.center_height();
  assert(lcd.rectangles.front() == (std::array<int,5>{
      popup_layout.x+(popup_layout.width-popup_width)/2,
      popup_layout.y+(popup_layout.height-popup_height)/2,
      popup_width,6,TFT_DARKGREEN}));
  std::vector<bool> popup_rows(popup_height,false);
  const int popup_top = popup_layout.y+(popup_layout.height-popup_height)/2;
  for (const auto &rectangle : lcd.rectangles) {
    if (rectangle[4] != TFT_DARKGREEN) continue;
    assert(rectangle[2] == popup_width && rectangle[3] <= 8);
    for (int y=rectangle[1]; y<rectangle[1]+rectangle[3]; ++y)
      popup_rows.at(y-popup_top) = true;
  }
  assert(std::all_of(popup_rows.begin(),popup_rows.end(),[](bool row) { return row; }));
  assert(lcd.menu_value_text.back() == "52%");
  advance(25); apply_encoder_step(1); service_encoder_menu();
  assert(lcd.menu_value_text.back() == "54%");
  advance(999); service_encoder_menu(); assert(encoder_control_popup.active);
  advance(1); service_encoder_menu();
  assert(!encoder_control_popup.active && menu_clears == clears_before_popup+1);
  encoder_control_attributes[static_cast<unsigned>(EncoderFunction::Master)].popup_enabled = false;
  apply_encoder_step(1);
  service_encoder_menu();
  assert(!encoder_control_popup.active && configuration.usb_play_volume == 56);
  encoder_control_attributes[static_cast<unsigned>(EncoderFunction::Master)].popup_enabled = true;
  configuration.usb_play_volume = 100;
  apply_encoder_step(1);
  assert(!encoder_control_popup.active); // At the range limit there was no change.
  open_encoder_menu(0);
  apply_encoder_step(-1);
  assert(!encoder_control_popup.active); // Full menu already shows the live value.
  close_encoder_menu();
  encoder_function = EncoderFunction::Vfo;
  lcd.rectangles.clear();
  apply_encoder_step(1); service_encoder_menu();
  assert(!encoder_control_popup.active && lcd.rectangles.empty()); // VFO never pops up.
  close_encoder_control_popup();
  encoder_function = EncoderFunction::Master;
  apply_encoder_step(-1);
  open_encoder_menu(0);
  assert(!encoder_control_popup.active);
  close_encoder_menu();
  configuration.usb_play_volume = 50;
  apply_encoder_step(1); service_encoder_menu();
  {
  const int touch_x = popup_layout.x+(popup_layout.width-popup_width)/2+5;
  const int touch_y = popup_layout.y+(popup_layout.height-popup_height)/2+5;
  assert(encoder_menu_touch(true,touch_x,touch_y));
  assert(encoder_menu_touch(true,touch_x+16,touch_y));
  assert(configuration.usb_play_volume == 56);
  assert(encoder_menu_touch(false,touch_x+16,touch_y));
  close_encoder_control_popup();
  assert(!encoder_menu_touch(true,touch_x,touch_y));
  assert(configuration.usb_play_volume == 56);
  encoder_menu_touch(false,touch_x,touch_y);
  open_encoder_menu(0);
  const auto vfo_touch_layout = encoder_menu_layout();
  const int vfo_index = 0;
  assert(encoder_menu_touch(true,vfo_touch_layout.cell_x(vfo_index)+5,
      vfo_touch_layout.cell_y(vfo_index)+5));
  assert(!encoder_menu.open && encoder_function == EncoderFunction::Vfo);
  const auto before_vfo_drag = outgoing.size();
  assert(encoder_menu_touch(true,touch_x+40,touch_y));
  assert(outgoing.size() == before_vfo_drag);
  encoder_menu_touch(false,touch_x,touch_y);
  apply_encoder_step(1);
  assert(encoder_function == EncoderFunction::Vfo && !encoder_control_popup.active);
  }
  const std::array<uint16_t,4> powers = {0x199,0x1E1,0x1F9,0x181};
  for (unsigned route = 0; route < 4; ++route) {
    assert(wm8960_audio_set_output_route(static_cast<Wm8960OutputRoute>(route)));
    assert(codec_writes.back() == (std::array<uint16_t,2>{0x1A,powers[route]}));
  }
  codec_write_ok = false;
  assert(!wm8960_audio_set_output_route(Wm8960OutputRoute::Both));
  assert(wm8960_audio_output_route() == Wm8960OutputRoute::None);
  codec_write_ok = true;
  assert(!wm8960_audio_set_output_route(static_cast<Wm8960OutputRoute>(4)));
  assert(cli_rx_audio(&terminal,"OUTPUT HEADPHONE"));
  assert(wm8960_audio_output_route() == Wm8960OutputRoute::Headphone);
  assert(cli_rx_audio(&terminal,"OUTPUT INVALID"));
  assert(wm8960_audio_output_route() == Wm8960OutputRoute::Headphone);
  assert(cli_rx_audio(&terminal,"RXTHRESHOLD 20"));
  assert(cli_rx_audio(&terminal,"RXFREQ 750"));
  assert(cli_rx_audio(&terminal,"RXWPM 40"));
  assert(cli_rx_audio(&terminal,"RXCHANNEL 1"));
  assert(wm8960_audio_rx_status().threshold == 20 &&
      wm8960_audio_rx_status().frequency == 750 &&
      wm8960_audio_rx_status().fixed_wpm == 40 &&
      wm8960_audio_rx_status().channel == 1);
  for (const char *invalid : {"RXTHRESHOLD -1","RXTHRESHOLD 101",
      "RXTHRESHOLD 1x", "RXFREQ 299","RXWPM 4","RXCHANNEL 2"}) {
    terminal.output.clear();
    assert(cli_rx_audio(&terminal,invalid));
    assert(terminal.output.find("Error:") != std::string::npos);
    assert(wm8960_audio_rx_status().threshold == 20);
  }
  lockable = false;
  assert(!wm8960_audio_configure_rx(5,600,20,0));
  assert(wm8960_audio_rx_status().threshold == 20);
  lockable = true;
  assert(cli_rx_audio(&terminal,"RXTHRESHOLD 0"));
  assert(wm8960_audio_rx_status().threshold == 0);
  encoder_function = EncoderFunction::RxThreshold;
  apply_encoder_step(1);
  assert(wm8960_audio_rx_status().threshold == 1);
  apply_encoder_step(-1);
  assert(wm8960_audio_rx_status().threshold == 0);
  for (uint8_t encoder = 0; encoder < 4; ++encoder) {
    const String assignment = String("ENCODER ") + String(encoder+1) + " RXTHRESHOLD";
    assert(cli_encoder_assignment(&terminal, assignment));
    assert(encoder_function_owner[static_cast<unsigned>(EncoderFunction::RxThreshold)] == encoder);
    assert(encoder_controls[encoder].function == EncoderFunction::RxThreshold);
    open_encoder_menu(encoder);
    assert(encoder_menu.count >= 1);
    for (uint8_t index=0; index<encoder_menu.count; ++index)
      assert(encoder_function_owner[encoder_menu.controls[index]] == encoder);
    close_encoder_menu();
    auto pins_for = [&](int value) {
      pin_values[pico_encoder_pins[encoder][0]] = value & 1;
      pin_values[pico_encoder_pins[encoder][1]] = (value >> 1) & 1;
      advance(300);
      encoder_irqs[pico_encoder_pins[encoder][0]]();
    };
    pins_for(3);
    for (int value : {2,0,1,3}) pins_for(value);
    check_rotary_encoder();
    assert(wm8960_audio_rx_status().threshold == static_cast<unsigned>((encoder+1)*steps_per_turn));
    check_encoder_switch(encoder);
    assert(encoder_has_function(encoder,encoder_controls[encoder].function));
  }
  const auto previous_owner = encoder_function_owner[0];
  assert(cli_encoder_assignment(&terminal,"ENCODER 5 VFO"));
  assert(cli_encoder_assignment(&terminal,"ENCODER 2 BOGUS"));
  assert(encoder_function_owner[0] == previous_owner);
  assert(cli_encoder_assignment(&terminal,"ENCODER 1 ALL"));
  for (uint8_t owner : encoder_function_owner) assert(owner == 0);
  for (uint8_t encoder = 1; encoder < 4; ++encoder)
    assert(encoder_controls[encoder].function == EncoderFunction::Count);
  reset_display_scroll_text();
  display_scroll_print_char('T');
  terminal.open = true;
  lcd_status = LCD_CLEAR;
  const int clears_before_rx = menu_clears;
  advance(100);
  const int previous_backlight_calls = backlight_on_calls;
  emit_usb_cw_character(111);
  assert(menu_clears == clears_before_rx && lcd_scroll_needs_clear);
  assert(lcd_status == LCD_SCROLL_MSG);
  assert(last_active_time == millis() && backlight_on_calls == previous_backlight_calls+1);
  assert(lcd_scroll_colors[0][0] == TFT_WHITE);
  assert(lcd_scroll_colors[0][1] == WM8960_RX_TEXT_COLOR);
  tft_draw_scroll_char(0,0); tft_draw_scroll_char(0,1);
  assert(lcd.scroll_glyphs[lcd.scroll_glyphs.size()-2] == (std::array<int,2>{'T',TFT_WHITE}));
  assert(lcd.scroll_glyphs.back() == (std::array<int,2>{'S',WM8960_RX_TEXT_COLOR}));
  for (unsigned i=0; i<LCD_ROWS*LCD_COLUMNS; ++i)
    display_scroll_print_char_color('R',WM8960_RX_TEXT_COLOR);
  for (byte row=0; row<LCD_ROWS; ++row)
    for (byte col=0; col<lcd_scroll_buffer[row].length(); ++col)
      assert(lcd_scroll_colors[row][col] == WM8960_RX_TEXT_COLOR);
  display_scroll_print_char('L');
  assert(lcd_scroll_colors[lcd_scroll_row_pointer][lcd_scroll_column_pointer-1] == TFT_WHITE);
  reset_display_scroll_text();
  assert(lcd_scroll_buffer[0].length() == 0 && lcd_scroll_colors[0][0] == TFT_WHITE);
  // Local RX changes only local playback, never PC POD levels or decoder settings.
  fresh();
  configuration.midi_control_mode = 2;
  assert(wm8960_midi_set_control_mode(2));
  configuration.usb_play_volume = 100;
  configuration.usb_play_muted = false;
  configuration.sidetone_volume = 50;
  configuration.sidetone_muted = false;
  configuration.usb_mic_volume = 60;
  configuration.usb_mic_muted = false;
  sync_pod_volume();
  assert(apply_audio_levels());
  service_keyer_midi();
  outgoing.clear();
  const auto rx_before_local = wm8960_audio_rx_status();
  encoder_function = EncoderFunction::LocalRx;
  apply_encoder_step(-1);
  assert(configuration.usb_play_volume == 98 && pod_play_volume == 100);
  assert(local_playback_gain_q15 == 98*32767/100);
  assert(local_sidetone_gain_q15 == 50*32767/100 && local_microphone_gain_q15 == 60*32767/100);
  assert(encoder_control_value_text(EncoderFunction::LocalRx,98).str() == "98%");
  service_keyer_midi(); assert(outgoing.empty());
  assert(cli_audio_volume(&terminal,"LOCALRX 15"));
  assert(configuration.usb_play_volume == 15 && local_playback_gain_q15 == 15*32767/100);
  assert(pod_play_volume == 100 && !pod_play_muted);
  assert(cli_audio_volume(&terminal,"LOCALRX MUTE"));
  assert(local_playback_gain_q15 == 0 && audio_muted(AudioVolumeTarget::LocalPlayback));
  assert(!audio_muted(AudioVolumeTarget::Playback));
  assert(cli_audio_volume(&terminal,"LOCALRX UNMUTE"));
  assert(local_playback_gain_q15 == 15*32767/100);
  auto rendered_playback = [&](const char *setting) {
    assert(cli_audio_volume(&terminal,setting));
    tone_active = midi_tone_active = false;
    tone_edge_level = 0;
    tone_mute_pending = false;
    local_voice_ptt = external_voice_ptt = false;
    playback_read_index = 0; playback_write_index = 256;
    playback_started = true; playback_mute_level = kMuteRampFrames;
    std::fill(std::begin(playback_ring),std::end(playback_ring),12000);
    render_i2s_output();
    assert(!wm8960_i2s::output.empty());
    return wm8960_i2s::output.front();
  };
  assert(rendered_playback("LOCALRX 100") == 12000);
  assert(rendered_playback("LOCALRX 25") >= 2999 && wm8960_i2s::output.front() <= 3000);
  assert(rendered_playback("LOCALRX MUTE") == 0);
  assert(cli_audio_volume(&terminal,"LOCALRX 15"));
  assert(cli_audio_volume(&terminal,"LOCALRX 101"));
  assert(configuration.usb_play_volume == 15);
  service_keyer_midi(); assert(outgoing.empty());
  set_audio_volume(AudioVolumeTarget::Playback,90,false);
  assert(pod_play_volume == 90 && configuration.usb_play_volume == 15);
  const auto rx_after_local = wm8960_audio_rx_status();
  assert(rx_after_local.threshold == rx_before_local.threshold &&
      rx_after_local.frequency == rx_before_local.frequency && rx_after_local.fixed_wpm == rx_before_local.fixed_wpm);
  for (uint8_t mode : {0,1}) {
    configuration.midi_control_mode = mode;
    assert(wm8960_midi_set_control_mode(mode));
    assert(cli_audio_volume(&terminal,"LOCALRX 25"));
    assert(configuration.usb_play_volume == 25 && local_playback_gain_q15 == 25*32767/100);
    assert(audio_volume(AudioVolumeTarget::Playback) == 25);
  }
  configuration.midi_control_mode = 1;
  close_encoder_menu();
  close_encoder_control_popup();
  puts("PASS: Local RX encoder/CLI local gain and mute in every mode; PC POD levels and decoder settings unchanged");
  lcd.scroll_positions.clear();
  CwRxDecoder repaint_stream;
  assert(repaint_stream.configure(50,700,20));
  repaint_stream.process(1280,[](long) {});
  lcd.scroll_draw_us = 40000;
  uint32_t max_repaint_gap_us = 0;
  auto draw_rx_text = [&]() {
    if (lcd_scroll_flag) {
      lcd_scroll_flag = 0;
      lcd_scroll_repaint_row = 0;
      lcd_scroll_repaint_col = 255;
    }
    for (int pass=0; pass<LCD_ROWS*(LCD_COLUMNS+2)+2; ++pass) {
      const auto glyphs_before = lcd.scroll_positions.size();
      const auto started = micros();
      const bool complete = tft_draw_pending_scroll_chars(2);
      assert(lcd.scroll_positions.size() - glyphs_before <= 2);
      const auto elapsed = micros() - started;
      max_repaint_gap_us = std::max(max_repaint_gap_us,elapsed);
      assert(elapsed <= 80000);
      std::vector<int16_t> incoming(elapsed / 1000 * 48 * 2);
      repaint_stream.submit(incoming.data(),incoming.size()/2,0);
      repaint_stream.process(1280,[](long) {});
      assert(repaint_stream.dropped() == 0 && repaint_stream.pending_samples() == 0);
      if (complete) {
        lcd_scroll_buffer_dirty = false;
        return;
      }
    }
    assert(false);
  };
  draw_rx_text();
  for (unsigned i=0; i<LCD_ROWS*LCD_COLUMNS*3; ++i) {
    const auto before_rx_draw = lcd.scroll_positions.size();
    emit_usb_cw_character(111);
    draw_rx_text();
    assert(!lcd_scroll_buffer_dirty);
    assert(lcd.scroll_positions.size() > before_rx_draw);
    assert(lcd.scroll_positions.back()[0] == 'S');
    assert(lcd.scroll_positions.back()[3] == WM8960_RX_TEXT_COLOR);
    assert(lcd.scroll_positions.back()[1] ==
        SCROLL_TEXT_LEFT_SIDE+(lcd_scroll_column_pointer-1)*COLUMN_WIDTH);
    assert(lcd.scroll_positions.back()[2] == tft_scroll_row_y(lcd_scroll_row_pointer));
  }
  assert(lcd_scroll_row_pointer == LCD_ROWS-1 && lcd_scroll_column_pointer == LCD_COLUMNS);
  emit_usb_cw_character(0);
  emit_usb_cw_character(111);
  draw_rx_text();
  assert(lcd_scroll_row_pointer == LCD_ROWS-1 && lcd_scroll_column_pointer == 1);
  assert(lcd_scroll_buffer[LCD_ROWS-1].str() == "S");
  reset_display_scroll_text();
  draw_rx_text();
  for (unsigned row=0; row<LCD_ROWS; ++row)
    for (unsigned col=0; col<LCD_COLUMNS; ++col)
      display_scroll_print_char('A'+row);
  draw_rx_text();
  emit_usb_cw_character(111);
  draw_rx_text();
  assert(lcd_scroll_buffer[0].str() == std::string(LCD_COLUMNS,'B'));
  assert(lcd_scroll_buffer[1].str() == std::string(LCD_COLUMNS,'C'));
  assert(lcd_scroll_buffer[2].str() == "S");
  assert(lcd_scroll_colors[0][0] == TFT_WHITE &&
      lcd_scroll_colors[2][0] == WM8960_RX_TEXT_COLOR);
  assert(max_repaint_gap_us == 80000);
  lcd.scroll_draw_us = 0;
  puts("PASS: slow TFT repaint/append stays within 80 ms per pass with zero PCM drops");
  puts("PASS: RX TFT scrolling beyond three full boxes, row shifts/colors, word-space wrap, empty repaint recovery");
  const auto saved_rx = wm8960_audio_rx_status();
  assert(wm8960_audio_configure_rx(50,700,20,0));
  rx_service_seen = true;
  rx_foreground_stage = 0;
  rx_foreground_max_us = 0;
  wm8960_audio_trace_keyer(15);
  clock_us += 600000;
  wm8960_audio_trace_keyer(16);
  auto foreground = wm8960_audio_rx_status();
  assert(foreground.max_foreground_stage == 15 && foreground.max_foreground_us == 600000);
  wm8960_audio_trace_keyer(17);
  clock_us += 650000;
  wm8960_audio_trace_keyer(18);
  foreground = wm8960_audio_rx_status();
  assert(foreground.max_foreground_stage == 17 && foreground.max_foreground_us == 650000);
  assert(wm8960_audio_configure_rx(saved_rx.threshold,saved_rx.frequency,saved_rx.fixed_wpm,saved_rx.channel));
  puts("PASS: longest foreground diagnostic distinguishes display and BOOTSEL stalls");
  terminal_wait_test::run();
  scroll_clear_test::run();
  display_guard_test::run();
  lcd.simulate_menu_latency = true;
  cooperative_max_gap_us = 0;
  cooperative_last_us = micros();
  in_pairing_mode = 0;
  key_state = ptt_line_activated = 0;
  tone_active = false;
  open_encoder_menu(0);
  advance(25);
  service_usb_cw_decoder();
  service_encoder_menu();
  advance(25);
  service_usb_cw_decoder();
  service_encoder_menu();
  assert(cooperative_max_gap_us < 50000);
  draw_encoder_control_box(EncoderFunction::RxSpeed,20,
      popup_layout.x+(popup_layout.width-popup_width)/2,
      popup_layout.y+(popup_layout.height-popup_height)/2,
      popup_width,popup_height,TFT_DARKGREEN);
  service_usb_cw_decoder();
  assert(cooperative_max_gap_us < 50000);
  lcd.simulate_menu_latency = false;
  close_encoder_menu();
  puts("PASS: slow encoder menu and RX WPM popup redraw services RX within 50 ms");
  // Nonblank mount failures cannot erase data. An erased new partition can initialize.
  flash.fill(0);
  assert(!encoder_settings_begin() && LittleFS.formats == 0);
  assert(flash_pauses == flash_resumes && !audio_flash_paused);
  flash.fill(0xFF);
  initialize_encoder_settings();
  assert(encoder_settings_ready && LittleFS.formats == 1);
  assert(filesystem.empty());
  assert(cli_encoder_assignment(&terminal,"ENCODER 2 RXFREQ"));
  encoder_controls[1].selection_pulses = 3;
  encoder_controls[0].function = EncoderFunction::Output;
  encoder_control_attributes[1].popup_enabled = false;
  assert(wm8960_audio_configure_rx(31,875,0,1));
  assert(wm8960_audio_set_output_route(Wm8960OutputRoute::Headphone));
  const EncoderSettings expected = current_encoder_settings();
  assert(valid_encoder_settings(expected));
  const auto encoded = encode_encoder_settings(expected);
  EncoderSettings decoded{};
  assert(decode_encoder_settings(encoded,decoded) && decoded == expected);
  for (size_t byte=0; byte<encoded.size(); ++byte) {
    auto damaged = encoded; damaged[byte] ^= 1;
    assert(!decode_encoder_settings(damaged,decoded));
  }
  for (const auto invalid : {std::array<unsigned,2>{0,4}, {11,12}, {15,0},
       {19,2}, {30,101}, {31,0}, {33,4}, {34,2}, {35,4}}) {
    auto damaged = expected; damaged[invalid[0]] = invalid[1];
    // Force the frequency below 300 for this case.
    if (invalid[0] == 31) damaged[32] = 0;
    assert(!valid_encoder_settings(damaged));
  }
  service_encoder_settings();
  advance(1999); service_encoder_settings(); assert(filesystem.empty());
  ptt_line_activated = 1;
  advance(5000); service_encoder_settings(); assert(filesystem.empty());
  ptt_line_activated = 0;
  encoder_movement_ms[0] = millis();
  service_encoder_settings(); assert(filesystem.empty());
  advance(2000); service_encoder_settings();
  bool found = false;
  assert(encoder_settings_load(decoded,found) && found && decoded == expected);
  assert(flash_pauses == flash_resumes && !audio_flash_paused);
  // Reboot restores values, assignments, sensitivity and current selection, not menu state.
  for (auto &owner : encoder_function_owner) owner = 0;
  for (uint8_t i=0; i<4; ++i) select_encoder_function(i,true);
  assert(wm8960_audio_configure_rx(0,600,20,0));
  assert(wm8960_audio_set_output_route(Wm8960OutputRoute::None));
  initialize_encoder_settings();
  assert(current_encoder_settings() == expected && !encoder_menu.open);
  encoder_controls[0].function = EncoderFunction::Master;
  service_encoder_settings(); advance(5000);
  fs_write_ok = false;
  service_encoder_settings();
  assert(encoder_settings_load(decoded,found) && decoded == expected);
  const auto attempts = flash_pauses;
  advance(4999); service_encoder_settings(); assert(flash_pauses == attempts);
  fs_write_ok = true; fs_rename_ok = false;
  advance(1); service_encoder_settings();
  assert(encoder_settings_load(decoded,found) && decoded == expected);
  fs_rename_ok = true; fs_close_ok = false;
  advance(5000); service_encoder_settings();
  assert(encoder_settings_load(decoded,found) && decoded == expected);
  fs_close_ok = true;
  advance(5000); service_encoder_settings();
  assert(encoder_settings_load(decoded,found) && decoded == current_encoder_settings());
  assert(flash_pauses == flash_resumes && !audio_flash_paused);
  // A version-1 file preserves old bindings/selections and adds Local RX on encoder 1.
  LegacyEncoderSettingsRecord legacy{};
  legacy[0]='K'; legacy[1]='M'; legacy[2]='S'; legacy[3]=1;
  for (unsigned function=0; function<10; ++function) {
    legacy[4+function] = expected[function];
    legacy[4+18+function] = expected[19+function];
  }
  for (unsigned encoder=0; encoder<4; ++encoder) {
    legacy[4+10+encoder] = expected[11+encoder] == 11 ? 10 : expected[11+encoder];
    legacy[4+14+encoder] = expected[15+encoder];
  }
  for (unsigned i=0; i<6; ++i) legacy[4+28+i] = expected[30+i];
  const auto legacy_checksum = encoder_settings_checksum(legacy.data(),38);
  for (unsigned i=0; i<4; ++i) legacy[38+i] = legacy_checksum >> (8*i);
  filesystem["/keyer-menu.bin"].data.assign(legacy.begin(),legacy.end());
  initialize_encoder_settings();
  assert(encoder_settings_ready && current_encoder_settings() == expected);
  assert(encoder_function_owner[10] == 0);
  auto bad_legacy = legacy;
  bad_legacy[3] = 3;
  assert(!decode_encoder_settings(bad_legacy,decoded));
  for (unsigned i=0; i<legacy.size(); ++i) {
    bad_legacy = legacy; bad_legacy[i] ^= 1;
    assert(!decode_encoder_settings(bad_legacy,decoded));
  }
  // If encoder 1 previously had no controls, its NONE selection becomes Local RX.
  for (unsigned function=0; function<10; ++function) legacy[4+function] = 1;
  legacy[4+10] = 10;
  legacy[4+11] = 0;
  legacy[4+12] = legacy[4+13] = 10;
  const auto empty_owner_checksum = encoder_settings_checksum(legacy.data(),38);
  for (unsigned i=0; i<4; ++i) legacy[38+i] = empty_owner_checksum >> (8*i);
  assert(decode_encoder_settings(legacy,decoded) && valid_encoder_settings(decoded));
  assert(decoded[10] == 0 && decoded[11] == 10 && decoded[12] == 0);
  filesystem["/keyer-menu.bin"].data[0] ^= 1;
  const auto damaged_file = filesystem["/keyer-menu.bin"].data;
  encoder_settings_ready = false;
  initialize_encoder_settings();
  assert(!encoder_settings_ready && filesystem["/keyer-menu.bin"].data == damaged_file);
  puts("PASS: menu persistence roundtrip, validation, idle/settle guards, atomic replacement failures, safe blank initialization");
  puts("PASS: audio controls, four-encoder capture/bindings, Select/Adjust menu, touch/drag/highlight/timeout, RX settings, codec routes");
}
'@

$checks = $checks.Replace('  display_guard_test::run();',
    "  display_guard_test::run();`n" + $keyboardPttRuns)
$temp = Join-Path ([IO.Path]::GetTempPath()) ("keyer-midi-test-" + [guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $temp
$source = Join-Path $temp 'test.cpp'
$exe = Join-Path $temp 'test.exe'
$obj = Join-Path $temp 'test.obj'
try {
    Set-Content -LiteralPath $source -Value (
        $mock + "`n" + $rxHeader + "`n" + $rxGlobals + "`n" + $sine + "`n" + ($audioFunctions -join "`n") + "`n" + $terminalWaitTest + "`n" + $clearTest + "`n" + $displayGuardTest + "`n" + $keyboardPttTest +
        "`n" + $header + "`n" + $midi + "`n" + $config +
        "`n" + $midiStatus + "`n" + $cli + "`n" + $potGuard + "`n" + $encoderMocks +
        "`n" + $keyInterrupts + "`n" + $encoderDirection + "`n" + $encoderTable + "`n" + $encoderCapture +
        "`n" + $encoder + "`n" + $rxCli + "`n" + $encoderCli +
        "`nuint32_t cooperative_last_us = 0, cooperative_max_gap_us = 0;`nvoid service_usb_cw_decoder() { cooperative_max_gap_us = std::max(cooperative_max_gap_us,micros()-cooperative_last_us); cooperative_last_us = micros(); }`n" +
        (Get-CppFunction $main 'tft_fill_rect_cooperative') +
        "`n" + ($encoderFunctions -join "`n") + "`n" + ($scrollFunctions -join "`n") +
        "`n" + $menuStorageMocks + "`n" + $menuStorage + "`n" + $menuSettings + "`n" + $checks
    ) -Encoding ASCII
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ tools required' }
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual Studio C++ tools not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    $diagnosticsObj = Join-Path $temp 'keyer_diagnostics.obj'
    & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W3 /c /Fo`"$diagnosticsObj`" `"$root\main\keyer_diagnostics.cpp`""
    if ($LASTEXITCODE -ne 0) { throw 'Diagnostics module compilation failed' }
    $gpsObj = Join-Path $temp 'keyer_gps.obj'
    & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W3 /c /Fo`"$gpsObj`" `"$root\main\keyer_gps.cpp`""
    if ($LASTEXITCODE -ne 0) { throw 'GPS module compilation failed' }
    $variants = if ($CwEncodersOnly) {
        @('', '/Drotary_switch=0', '/Drotary_switch=42', '/DENCODER_VFO_DIRECTION=1', '/DTEST_ENCODER_FULL_STEP', '/DWM8960_RX_TEXT_COLOR=0xFFE0')
    } else {
        @('', '/DDEBUG_MIDI', '/DDEBUG_MIDI_MESSAGES', '/DDEBUG_MIDI /DDEBUG_MIDI_MESSAGES', '/Drotary_switch=0', '/Drotary_switch=42', '/DENCODER_VFO_DIRECTION=1', '/DTEST_ENCODER_FULL_STEP', '/DWM8960_MIC_GATE_MODE=0', '/DWM8960_MIC_GATE_MODE=1', '/DWM8960_MIC_GATE_MODE=2', '/DWM8960_PTT_FEEDBACK_CHANNEL=10', '/DWM8960_PTT_FEEDBACK_CHANNEL=10 /DWM8960_PTT_FEEDBACK_TYPE=9', '/DWM8960_PTT_FEEDBACK_CHANNEL=10 /DWM8960_PTT_FEEDBACK_TYPE=9 /DWM8960_PTT_FEEDBACK_NUMBER=19', '/DWM8960_MIC_LEVEL_FEEDBACK_CHANNEL=10', '/DWM8960_MIC_LEVEL_FEEDBACK_CHANNEL=10 /DDEBUG_MIDI_MESSAGES')
    }
    foreach ($defines in $variants) {
        if ($CwEncodersOnly) { $defines += ' /DTEST_CW_ENCODERS_ONLY' }
        & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W3 /I`"$root\main`" $defines /Fo`"$obj`" /Fe`"$exe`" `"$source`" `"$diagnosticsObj`" `"$gpsObj`""
        if ($LASTEXITCODE -ne 0) { throw "Host compilation failed: $defines" }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw "Host assertions failed: $defines" }
    }
} finally {
    foreach ($file in @($source, $exe, $obj, (Join-Path $temp 'keyer_diagnostics.obj'), (Join-Path $temp 'keyer_gps.obj'))) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    Remove-Item -LiteralPath $temp
}
