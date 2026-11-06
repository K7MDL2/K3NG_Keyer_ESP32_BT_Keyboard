$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$temp = Join-Path ([IO.Path]::GetTempPath()) ("keyer-platform-test-" + [guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $temp
$exe = Join-Path $temp 'test.exe'
$obj = Join-Path $temp 'test.obj'
$diagnosticsObj = Join-Path $temp 'diagnostics.obj'
try {
    & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W4 /c /Fo`"$diagnosticsObj`" `"$root\main\keyer_diagnostics.cpp`""
    if ($LASTEXITCODE -ne 0) { throw 'Diagnostics compilation failed' }
    $targets = @(
        '/DTEST_ESP32 /DPROJECT_ESP32_COMPILER',
        '/DTEST_ESP32 /DARDUINO_ESP32_DEV',
        '/DTEST_ESP32 /DARDUINO_ARCH_ESP32',
        '/DTEST_PICO /DARDUINO_ARCH_RP2040',
        '/DTEST_PICO /DTEST_WIRELESS /DARDUINO_ARCH_RP2040 /DPICO_CYW43_SUPPORTED',
        '/DTEST_PICO /DPICO_PLATFORM',
        '/DTEST_PICO /DTEST_WIRELESS /DPICO_PLATFORM /DPICO_CYW43_SUPPORTED',
        '/DTEST_TEENSY /DTEENSYDUINO /D__IMXRT1062__ /DARDUINO_TEENSY40',
        '/DTEST_TEENSY /DTEENSYDUINO /D__IMXRT1062__ /DARDUINO_TEENSY41',
        '/DTEST_ESP32 /DPROJECT_ESP32_COMPILER /DOPTION_BT_KEYBOARD_US',
        '/DTEST_ESP32 /DPROJECT_ESP32_COMPILER /DOPTION_BT_KEYBOARD_GERMAN',
        '/DTEST_ESP32 /DPROJECT_ESP32_COMPILER /DOPTION_BT_KEYBOARD_FRENCH',
        '/DTEST_ESP32 /DPROJECT_ESP32_COMPILER /DOPTION_PS2_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT'
    )
    foreach ($target in $targets) {
        & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W4 /I`"$root\main`" $target /DFLASH_SECTOR_SIZE=4096 /DPICO_FLASH_SIZE_BYTES=4194304 /Fo`"$obj`" /Fe`"$exe`" `"$PSScriptRoot\keyer_platform_host_test.cpp`" `"$diagnosticsObj`""
        if ($LASTEXITCODE -ne 0) { throw "Platform compilation failed: $target" }
        & $exe
        if ($LASTEXITCODE -ne 0) { throw "Platform assertions failed: $target" }
    }
    foreach ($target in @('', '/DARDUINO_AVR_UNO', '/DARDUINO_SAM_DUE', '/DARDUINO_SAMD_VARIANT_COMPLIANCE',
        '/DARDUINO_GENERIC_STM32F103C', '/D_BOARD_PIC32_PINGUINO_',
        '/DTEENSYDUINO /D__AVR__', '/DTEENSYDUINO /D__MK66FX1M0__')) {
        $output = & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /c /I`"$root\main`" $target /Fo`"$obj`" `"$PSScriptRoot\keyer_platform_host_test.cpp`"" 2>&1
        if ($LASTEXITCODE -eq 0 -or "$output" -notmatch 'Unsupported keyer CPU') {
            throw "Unsupported-target rejection failed: $target`n$output"
        }
    }
    foreach ($feature in @('FEATURE_PS2_KEYBOARD', 'FEATURE_USB_KEYBOARD')) {
        $output = & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /c /I`"$root\main`" /DPROJECT_ESP32_COMPILER /D$feature /Fo`"$obj`" `"$PSScriptRoot\keyer_platform_host_test.cpp`"" 2>&1
        if ($LASTEXITCODE -eq 0 -or "$output" -notmatch 'use FEATURE_BT_KEYBOARD') {
            throw "Removed keyboard feature must be rejected: $feature`n$output"
        }
    }
    Write-Host 'PASS: nine platform selectors, three BT layouts, legacy LCD option alias, eight unsupported targets, two removed keyboard features, unchanged Pico BT layout, all 256 diagnostic stage names'
} finally {
    foreach ($file in @($exe, $obj, $diagnosticsObj)) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    Remove-Item -LiteralPath $temp
}
