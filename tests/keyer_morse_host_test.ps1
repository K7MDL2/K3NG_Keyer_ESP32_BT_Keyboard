$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$main = Get-Content (Join-Path $root 'main\main.cpp') -Raw
$wrapper = [regex]::Match($main, '(?s)int convert_cw_number_to_ascii \(long number_in\)\s*\{.*?\r?\n\}').Value
if (!$wrapper) { throw 'Morse conversion wrapper not found' }
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$temp = Join-Path ([IO.Path]::GetTempPath()) ("keyer-morse-test-" + [guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $temp
$source = Join-Path $temp 'test.cpp'
$exe = Join-Path $temp 'test.exe'
$obj = Join-Path $temp 'test.obj'
$morseObj = Join-Path $temp 'keyer_morse.obj'
try {
    $mock = "#include `"keyer.h`"`n#include `"keyer_morse.h`"`n#define unknown_cw_character '#'`nint error_tones = 0;`nvoid boop() { ++error_tones; }`n"
    Set-Content -LiteralPath $source -Value ($mock + $wrapper + "`n#include `"$PSScriptRoot\keyer_morse_host_test.cpp`"") -Encoding ASCII
    & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W4 /c /Fo`"$morseObj`" `"$root\main\keyer_morse.cpp`""
    if ($LASTEXITCODE -ne 0) { throw 'Morse module compilation failed' }
    $options = @('OPTION_PROSIGN_SUPPORT', 'OPTION_NON_ENGLISH_EXTENSIONS',
        'OPTION_KEYBOARD_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT', 'OPTION_UNKNOWN_CHARACTER_ERROR_TONE')
    for ($variant = 0; $variant -lt 16; ++$variant) {
        $defines = ''
        for ($bit = 0; $bit -lt 4; ++$bit) {
            if ($variant -band (1 -shl $bit)) { $defines += " /D$($options[$bit])" }
        }
        & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W4 /I`"$root\main`" $defines /Fo`"$obj`" /Fe`"$exe`" `"$source`" `"$morseObj`""
        if ($LASTEXITCODE -ne 0) { throw "Morse wrapper compilation failed: $defines" }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw "Morse assertions failed: $defines" }
    }
    Write-Host 'PASS: all 16 Morse feature combinations linked against the separate module'
} finally {
    foreach ($file in @($source, $exe, $obj, $morseObj)) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    Remove-Item -LiteralPath $temp
}
