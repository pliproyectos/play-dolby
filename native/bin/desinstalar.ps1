# Desinstalador de Play Dolby. Quita el programa, sus accesos y su registro en «Aplicaciones instaladas».
# Conserva el perfil y la bitácora del usuario (%LOCALAPPDATA%\PlanetarioBridge).
param(
    [string]$Destino = $PSScriptRoot,
    [string]$Escritorio = [Environment]::GetFolderPath('Desktop'),
    [string]$MenuInicio = (Join-Path ([Environment]::GetFolderPath('Programs')) 'Play Dolby'),
    [string]$AlIniciar = [Environment]::GetFolderPath('Startup'),
    [switch]$SinRegistro,
    [switch]$Silencioso
)
$ErrorActionPreference = 'Stop'
try {
    if (-not (Test-Path (Join-Path $Destino 'PlayDolby.exe'))) { throw "No encuentro Play Dolby en $Destino." }
    # Protección: solo se borra una carpeta que se llame «Play Dolby» y tenga el programa.
    if ((Split-Path $Destino -Leaf) -ne 'Play Dolby') { throw "Por seguridad solo se borra una carpeta llamada «Play Dolby» (recibí $Destino)." }
    if (-not $Silencioso) {
        $r = Read-Host '  ¿Desinstalar Play Dolby de esta PC? (S/N)'
        if ($r -notmatch '^[sS]') { Write-Host '  No se cambió nada.'; exit 0 }
    }
    $abierto = Get-Process PlayDolby -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($Destino, [StringComparison]::OrdinalIgnoreCase) }
    if ($abierto) { throw 'Play Dolby está abierto. Ciérralo y vuelve a intentar.' }
    $acceso = Join-Path $Escritorio 'Play Dolby.lnk'
    if (Test-Path $acceso) { Remove-Item -LiteralPath $acceso -Force }
    if (Test-Path $MenuInicio) { Remove-Item -LiteralPath $MenuInicio -Recurse -Force }
    $revision = Join-Path $AlIniciar 'Play Dolby - buscar actualizaciones.lnk'
    if (Test-Path $revision) { Remove-Item -LiteralPath $revision -Force }
    if (-not $SinRegistro) { Remove-Item -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\PlayDolbyPLI' -Recurse -Force -ErrorAction SilentlyContinue }
    Set-Location $env:TEMP
    Remove-Item -LiteralPath $Destino -Recurse -Force
    Write-Host '  Play Dolby se desinstaló. Tu perfil y bitácora siguen en %LOCALAPPDATA%\PlanetarioBridge.' -ForegroundColor Green
    if (-not $Silencioso) { Read-Host '  Presiona Enter para cerrar' | Out-Null }
    exit 0
} catch {
    Write-Host ('  NO SE DESINSTALÓ: ' + $_.Exception.Message) -ForegroundColor Red
    if (-not $Silencioso) { Read-Host '  Presiona Enter para cerrar' | Out-Null }
    exit 1
}
