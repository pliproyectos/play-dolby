# Instalador de Play Dolby (PLI Ingeniería Aplicada) para Windows 10/11 de 64 bits.
# Instala en la cuenta del usuario: NO necesita permisos de administrador ni internet.
# Se lanza con INSTALAR.cmd (doble clic). Los parámetros existen para pruebas.
param(
    [string]$Destino = (Join-Path $env:LOCALAPPDATA 'Programs\Play Dolby'),
    [string]$Escritorio = [Environment]::GetFolderPath('Desktop'),
    [string]$MenuInicio = (Join-Path ([Environment]::GetFolderPath('Programs')) 'Play Dolby'),
    [switch]$SinRegistro,
    [switch]$Silencioso
)
$ErrorActionPreference = 'Stop'
$paquete = Split-Path $PSScriptRoot -Parent
$programa = Join-Path $paquete 'programa'
$version = 'sin versión'
if (Test-Path (Join-Path $programa 'VERSION.txt')) { $version = (Get-Content (Join-Path $programa 'VERSION.txt') | Select-Object -First 1) }

function Pausa { if (-not $Silencioso) { Write-Host ''; Read-Host 'Presiona Enter para cerrar' | Out-Null } }
function Paso([string]$t) { Write-Host ('  · ' + $t) }
function Acceso([string]$ruta, [string]$objetivo, [string]$argumentos, [string]$carpeta, [string]$icono, [string]$descripcion) {
    $sh = New-Object -ComObject WScript.Shell
    $l = $sh.CreateShortcut($ruta)
    $l.TargetPath = $objetivo
    if ($argumentos) { $l.Arguments = $argumentos }
    if ($carpeta) { $l.WorkingDirectory = $carpeta }
    if ($icono) { $l.IconLocation = $icono }
    $l.Description = $descripcion
    $l.Save()
}

try {
    Write-Host ''
    Write-Host "  PLAY DOLBY $version · PLI Ingeniería Aplicada" -ForegroundColor Magenta
    Write-Host '  Instalación para este usuario (sin administrador)'
    Write-Host ''
    if (-not [Environment]::Is64BitOperatingSystem) { throw 'Play Dolby necesita Windows de 64 bits.' }
    if (-not (Test-Path (Join-Path $programa 'PlayDolby.exe'))) { throw "No encuentro el programa en $programa. Corre INSTALAR.cmd desde la carpeta de la USB, sin moverlo." }
    $abierto = Get-Process PlayDolby -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($Destino, [StringComparison]::OrdinalIgnoreCase) }
    if ($abierto) { throw 'Play Dolby está abierto. Ciérralo y vuelve a correr el instalador.' }

    # Si ya estaba instalado, se conserva la ficha del domo (puede tener los ángulos medidos en sala).
    $ficha = Join-Path $Destino 'domo.json'
    $fichaAnterior = $null
    if (Test-Path $ficha) { $fichaAnterior = [IO.File]::ReadAllBytes($ficha) }

    Paso "Copiando el programa a $Destino"
    New-Item -ItemType Directory -Force -Path $Destino | Out-Null
    Copy-Item -Path (Join-Path $programa '*') -Destination $Destino -Recurse -Force
    if ($fichaAnterior) { [IO.File]::WriteAllBytes($ficha, $fichaAnterior); Paso 'Se conservó tu domo.json anterior' }
    $demo = Join-Path $paquete 'demo'
    if (Test-Path $demo) { Paso 'Copiando la demo: pruebas 7.1, música 7.1 y capas (≈ 2 GB, tarda un poco)'; Copy-Item -Path $demo -Destination $Destino -Recurse -Force }
    Copy-Item -Path (Join-Path $PSScriptRoot 'desinstalar.ps1') -Destination $Destino -Force
    Get-ChildItem -LiteralPath $Destino -Recurse -File | Unblock-File

    Paso 'Creando accesos directos'
    $exe = Join-Path $Destino 'PlayDolby.exe'
    $ps = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    $her = Join-Path $Destino 'herramientas'
    New-Item -ItemType Directory -Force -Path $Escritorio | Out-Null  # p. ej. Escritorio en OneDrive aún sin crear
    Acceso (Join-Path $Escritorio 'Play Dolby.lnk') $exe '' $Destino "$exe,0" 'Play Dolby · audio 7.1 y PLI Espacial'
    New-Item -ItemType Directory -Force -Path $MenuInicio | Out-Null
    Acceso (Join-Path $MenuInicio 'Play Dolby.lnk') $exe '' $Destino "$exe,0" 'Play Dolby · audio 7.1 y PLI Espacial'
    Acceso (Join-Path $MenuInicio 'Prueba de hardware (Qu-5 a HDMI).lnk') (Join-Path $her 'prueba-hardware.cmd') '' $her "$exe,0" 'Prueba de 2 minutos en silencio: consola a HDMI'
    Acceso (Join-Path $MenuInicio 'Diagnóstico de audio.lnk') $ps ('-NoProfile -NoExit -ExecutionPolicy Bypass -File "' + (Join-Path $her 'diagnostico-audio.ps1') + '"') $her '' 'Qué ve Windows: receptor, 7.1, mejoradores, ASIO'
    Acceso (Join-Path $MenuInicio 'Demo y pruebas 7.1.lnk') (Join-Path $Destino 'demo') '' '' '' 'Pruebas 7.1 de la sala, música en 7.1 y capas para PLI Espacial'
    Acceso (Join-Path $MenuInicio 'Léeme.lnk') (Join-Path $Destino 'LEEME.txt') '' '' '' 'Resumen rápido'
    Acceso (Join-Path $MenuInicio 'Guía de usuario.lnk') (Join-Path $Destino 'guia\GUIA-DE-USUARIO.md') '' '' '' 'Guía completa de Play Dolby, con capturas'
    Acceso (Join-Path $MenuInicio 'Desinstalar Play Dolby.lnk') $ps ('-NoProfile -ExecutionPolicy Bypass -File "' + (Join-Path $Destino 'desinstalar.ps1') + '"') $env:TEMP '' 'Quita Play Dolby de esta PC'

    if (-not $SinRegistro) {
        Paso 'Registrando en «Aplicaciones instaladas»'
        $clave = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\PlayDolbyPLI'
        New-Item -Path $clave -Force | Out-Null
        $kb = [int]((Get-ChildItem -LiteralPath $Destino -Recurse -File | Measure-Object Length -Sum).Sum / 1KB)
        $valores = @{ DisplayName = 'Play Dolby'; DisplayVersion = $version; Publisher = 'PLI Ingeniería Aplicada'; DisplayIcon = "$exe,0"
                      InstallLocation = $Destino; UninstallString = ('"' + $ps + '" -NoProfile -ExecutionPolicy Bypass -File "' + (Join-Path $Destino 'desinstalar.ps1') + '"') }
        foreach ($k in $valores.Keys) { New-ItemProperty -Path $clave -Name $k -Value $valores[$k] -PropertyType String -Force | Out-Null }
        foreach ($k in 'NoModify', 'NoRepair') { New-ItemProperty -Path $clave -Name $k -Value 1 -PropertyType DWord -Force | Out-Null }
        New-ItemProperty -Path $clave -Name 'EstimatedSize' -Value $kb -PropertyType DWord -Force | Out-Null
    }

    Write-Host ''
    Write-Host '  Controlador de la Qu-5 (solo se revisa, no se descarga nada):'
    & $ps -NoProfile -ExecutionPolicy Bypass -File (Join-Path $her 'check-qu5-driver.ps1') -CheckOnly -NoBrowser | ForEach-Object { Write-Host ('    ' + $_) }
    if ($LASTEXITCODE -ne 0) { Write-Host '    Sin la Qu-5 puedes usar la «fuente archivo» (ver LEEME). Controlador oficial: https://www.allen-heath.com/hardware/qu/qu-5-qu-5d/resources/' }

    Write-Host ''
    Write-Host '  LISTO. Play Dolby quedó instalado.' -ForegroundColor Green
    Write-Host '  Ábrelo con el acceso «Play Dolby» del Escritorio o del menú Inicio.'
    Write-Host '  Para quitarlo: Configuración > Aplicaciones > Aplicaciones instaladas > Play Dolby.'
    if (-not $Silencioso) {
        $r = Read-Host '  ¿Abrir Play Dolby ahora? (S/N)'
        if ($r -match '^[sS]') { Start-Process -FilePath $exe -WorkingDirectory $Destino }
    }
    exit 0
} catch {
    Write-Host ''
    Write-Host ('  NO SE INSTALÓ: ' + $_.Exception.Message) -ForegroundColor Red
    Pausa
    exit 1
}
