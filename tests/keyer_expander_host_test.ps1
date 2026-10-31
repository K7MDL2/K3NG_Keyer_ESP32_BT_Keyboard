$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$main = Get-Content (Join-Path $root 'main\main.cpp') -Raw
$pins = Get-Content (Join-Path $root 'main\keyer_pin_settings_pico.h') -Raw
$helpers = [regex]::Match($main, '(?s)#if defined\(ARDUINO_ARCH_RP2040\) && defined\(FEATURE_MCP23017_EXPANDER\)\r?\nuint8_t keyer_output_latches.*?(?=void ptt_key\(\))').Value
if (!$helpers) { throw 'TX/PTT helpers not found' }
$activeCode = [regex]::Replace($main, '(?m)//.*$', '')
if ($activeCode -match '\bdigitalWrite\s*\(\s*(?:tx_key_line_[1-6]|ptt_tx_[1-6]|current_tx_key_line|current_tx_ptt_line|configuration\.current_ptt_line)\b') {
    throw 'TX/PTT path bypasses output routing'
}

$mock = @'
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <array>
#include <vector>
#define ARDUINO_ARCH_RP2040
#define FEATURE_TFT_PICO_320x480_RES_LCD
#define FEATURE_STRAIGHT_KEY
constexpr uint8_t HIGH = 1, LOW = 0, OUTPUT = 1;
struct Write { uint8_t reg, value; };
struct TwoWire {
  std::vector<uint8_t> bytes;
  std::vector<Write> writes;
  uint8_t error = 0, address = 0;
  void beginTransmission(uint8_t addr) { address = addr; bytes.clear(); }
  void write(uint8_t byte) { bytes.push_back(byte); }
  uint8_t endTransmission() {
    assert(bytes.size() == 2);
    if (!error) writes.push_back({bytes[0],bytes[1]});
    return error;
  }
} Wire, Wire1;
struct Serial {
  int errors = 0;
  void printf(const char *, ...) { ++errors; }
} serial;
Serial *debug_serial_port = &serial;
std::vector<std::array<uint8_t,2>> cpu_writes, cpu_modes;
void digitalWrite(uint8_t pin, uint8_t state) { cpu_writes.push_back({pin,state}); }
void pinMode(uint8_t pin, uint8_t mode) { cpu_modes.push_back({pin,mode}); }
'@

$checks = @'
int main() {
  const uint8_t keys[6] = {tx_key_line_1,tx_key_line_2,tx_key_line_3,
      tx_key_line_4,tx_key_line_5,tx_key_line_6};
  const uint8_t ptts[6] = {ptt_tx_1,ptt_tx_2,ptt_tx_3,ptt_tx_4,ptt_tx_5,ptt_tx_6};
#ifdef FEATURE_MCP23017_EXPANDER
#ifdef USE_WIRE1
  TwoWire &bus = Wire1;
  TwoWire &other = Wire;
#else
  TwoWire &bus = Wire;
  TwoWire &other = Wire1;
#endif
  assert(paddle_left == 0 && paddle_right == 1 && pin_straight_key == 2);
  for (int tx = 0; tx < 6; ++tx) {
    assert(keys[tx] == 103 + tx * 2);
    assert(ptts[tx] == 104 + tx * 2);
  }
  for (uint8_t inactive : {LOW,HIGH}) {
    keyer_output_latches[0] = keyer_output_latches[1] = 0;
    keyer_output_directions[0] = keyer_output_directions[1] = 0xFF;
    bus.writes.clear();
    for (int tx = 0; tx < 6; ++tx) {
      for (uint8_t pin : {keys[tx],ptts[tx]}) {
        keyer_output_initialize(pin,inactive);
        assert(bus.writes[bus.writes.size()-2].reg == 0x14 + (pin-100)/8);
        assert(bus.writes.back().reg == (pin-100)/8);
      }
    }
    assert(keyer_output_directions[0] == 0x07);
    assert(keyer_output_directions[1] == 0x80);
    const uint8_t initial_a = inactive ? 0xF8 : 0;
    const uint8_t initial_b = inactive ? 0x7F : 0;
    assert(keyer_output_latches[0] == initial_a && keyer_output_latches[1] == initial_b);
    for (int tx = 0; tx < 6; ++tx) {
      for (uint8_t pin : {keys[tx],ptts[tx]}) {
        const uint8_t port = (pin-100)/8, mask = 1u << ((pin-100)%8);
        const uint8_t original = keyer_output_latches[port];
        keyer_output_write(pin,!inactive);
        assert(keyer_output_latches[port] == uint8_t(original ^ mask));
        keyer_output_write(pin,inactive);
        assert(keyer_output_latches[port] == original);
      }
    }
    assert(cpu_writes.empty() && cpu_modes.empty());
  }
  assert(other.writes.empty() && bus.address == MCP23X17_ADDR);
  const auto saved = keyer_output_latches[0];
  bus.error = 2; keyer_output_write(tx_key_line_1,HIGH);
  assert(keyer_output_latches[0] == saved && serial.errors == 1);
  const auto direction = keyer_output_directions[0];
  keyer_output_initialize(tx_key_line_1,HIGH);
  assert(keyer_output_directions[0] == direction && serial.errors == 2);
  bus.error = 0;
  keyer_output_initialize(14,LOW); keyer_output_write(14,HIGH);
  assert(cpu_modes.back()[0] == 14 && cpu_writes.back()[0] == 14);
#else
  assert(tx_key_line_1 == 0 && ptt_tx_1 == 0);
  for (int tx = 1; tx < 6; ++tx) assert(keys[tx] == 0 && ptts[tx] == 0);
  keyer_output_initialize(14,LOW);
  keyer_output_write(14,HIGH);
  keyer_output_initialize(22,HIGH);
  keyer_output_write(22,LOW);
  assert(cpu_modes.size() == 2 && cpu_writes.size() == 4);
  assert(cpu_writes[0][0] == 14 && cpu_writes[0][1] == LOW);
  assert(cpu_writes[1][0] == 14 && cpu_writes[1][1] == HIGH);
  assert(cpu_writes[2][0] == 22 && cpu_writes[2][1] == HIGH);
  assert(cpu_writes[3][0] == 22 && cpu_writes[3][1] == LOW);
  assert(Wire.writes.empty() && Wire1.writes.empty());
#endif
  puts("PASS: six TX/PTT mappings, output polarity, latch preservation, safe direction order, I2C failures, direct GPIO fallback");
}
'@

$temp = Join-Path ([IO.Path]::GetTempPath()) ("keyer-expander-test-" + [guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $temp
$source = Join-Path $temp 'test.cpp'
$exe = Join-Path $temp 'test.exe'
$obj = Join-Path $temp 'test.obj'
try {
    Set-Content -LiteralPath $source -Value ($mock + "`n" + $pins + "`n" + $helpers + "`n" + $checks) -Encoding ASCII
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio C++ tools required' }
    $vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Visual Studio C++ tools not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    foreach ($defines in @('', '/DFEATURE_MCP23017_EXPANDER', '/DFEATURE_MCP23017_EXPANDER /DUSE_WIRE1')) {
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
