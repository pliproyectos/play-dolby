param([string]$OutputName='PlayDolby.exe')
$ErrorActionPreference='Stop'
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $PSScriptRoot '.zig-cache-global'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $PSScriptRoot '.zig-cache-local'
Push-Location $PSScriptRoot
try {
    & (Join-Path $PSScriptRoot 'embed-logo.ps1')

    $compiler=Join-Path $PSScriptRoot 'tools\zig-x86_64-windows-0.14.1\zig.exe'
    & $compiler rc /fo bin/pli-app.res src/app_resources.rc
    if($LASTEXITCODE){throw 'Error al compilar el icono de Windows.'}
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra tests/core_tests.cpp -o bin/core_tests.exe
    if($LASTEXITCODE){throw 'Error al compilar las pruebas.'}
    & .\bin\core_tests.exe
    if($LASTEXITCODE){throw 'Fallaron las pruebas.'}
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra tests/channel_sequence_tests.cpp -o bin/channel_sequence_tests.exe
    if($LASTEXITCODE){throw 'Error al compilar las pruebas del probador.'}
    & .\bin\channel_sequence_tests.exe
    if($LASTEXITCODE){throw 'Fallaron las pruebas del probador.'}
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra -municode '-Wl,--subsystem,windows' src/main_cinematic.cpp bin/pli-app.res -o (Join-Path 'bin' $OutputName) -lgdiplus -lcomctl32 -lgdi32 -lole32 -luuid
    if($LASTEXITCODE){throw 'Error al compilar Play Dolby.'}
} finally {Pop-Location}
