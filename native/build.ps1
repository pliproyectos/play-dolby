$ErrorActionPreference='Stop'
Push-Location $PSScriptRoot
try {
    $compiler=Join-Path $PSScriptRoot 'tools\zig-x86_64-windows-0.14.1\zig.exe'
    if(!(Test-Path $compiler)){throw 'Falta compilador Zig 0.14.1 x86_64-windows en native/tools.'}
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra tests/core_tests.cpp -o bin/core_tests.exe
    if($LASTEXITCODE){throw 'Falló la compilación de las pruebas.'}
    & .\bin\core_tests.exe
    if($LASTEXITCODE){throw 'Fallaron las pruebas.'}
    & $compiler c++ -std=c++17 -O2 -Wall -Wextra -municode '-Wl,--subsystem,windows' src/main.cpp -o bin/PlanetarioBridge.exe -lgdiplus -lcomctl32 -lgdi32 -lole32 -luuid
    if($LASTEXITCODE){throw 'Falló la compilación de la aplicación.'}
} finally {Pop-Location}
