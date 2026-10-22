# PLI Espacial: compila y corre las pruebas del paneo y de escenas; si pasan, compila capas71,
# escena71 y upmix71.
# Uso: pwsh -File native\build-espacial.ps1
$ErrorActionPreference='Stop'
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $PSScriptRoot '.zig-cache-global'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $PSScriptRoot '.zig-cache-local'
Push-Location $PSScriptRoot
try {
    $compiler=Join-Path $PSScriptRoot 'tools\zig-x86_64-windows-0.14.1\zig.exe'
    if(-not (Test-Path $compiler)){throw 'Falta el compilador Zig. Corre primero: pwsh -File native\tools\obtener-zig.ps1'}
    New-Item -ItemType Directory -Force bin | Out-Null
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra tests/espacial_tests.cpp -o bin/espacial_tests.exe
    if($LASTEXITCODE){throw 'Error al compilar las pruebas de PLI Espacial.'}
    & .\bin\espacial_tests.exe ..\pruebas-domo\domo.json
    if($LASTEXITCODE){throw 'Fallaron las pruebas de PLI Espacial.'}
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra upmix/capas71.cpp -o bin/capas71.exe
    if($LASTEXITCODE){throw 'Error al compilar capas71.'}
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra espacial/escena71.cpp -o bin/escena71.exe
    if($LASTEXITCODE){throw 'Error al compilar escena71.'}
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra espacial/adm71.cpp -o bin/adm71.exe
    if($LASTEXITCODE){throw 'Error al compilar adm71.'}
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra upmix/upmix71.cpp -o bin/upmix71.exe
    if($LASTEXITCODE){throw 'Error al compilar upmix71.'}
    Write-Host 'PLI Espacial: pruebas y compilación OK.'
} finally {Pop-Location}
