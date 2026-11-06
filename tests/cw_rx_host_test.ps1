param([string]$RecordingPcm, [string]$ExpectedSubstring)
$ErrorActionPreference = 'Stop'
if ([bool]$RecordingPcm -ne [bool]$ExpectedSubstring) {
    throw 'Provide both -RecordingPcm and -ExpectedSubstring'
}
$root = Split-Path $PSScriptRoot -Parent
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio C++ tools required' }
$temp = Join-Path ([IO.Path]::GetTempPath()) ("keyer-cw-test-" + [guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $temp
$obj = Join-Path $temp 'test.obj'
$exe = Join-Path $temp 'test.exe'
try {
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    & $env:ComSpec /c "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W4 /I`"$root\main`" /Fo`"$obj`" /Fe`"$exe`" `"$PSScriptRoot\cw_rx_host_test.cpp`""
    if ($LASTEXITCODE -ne 0) { throw 'CW decoder host compilation failed' }
    if ($RecordingPcm) {
        & $exe $RecordingPcm $ExpectedSubstring
    } else {
        & $exe
    }
    if ($LASTEXITCODE -ne 0) { throw 'CW decoder assertions failed' }
} finally {
    foreach ($file in @($obj,$exe)) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
    Remove-Item -LiteralPath $temp
}
