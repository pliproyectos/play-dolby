# Actualizador de Play Dolby (modo CON AVISO). Corre en cada terminal instalada.
#  -Revisar : consulta el último Release público de GitHub (repositorio en actualizacion.json). Si hay
#             una versión más nueva, la baja, verifica su SHA-256 y AVISA (pregunta si instalar).
#             Si Play Dolby está abierto, no pregunta: Play Dolby muestra el botón «Actualización».
#  -Instalar: instala la versión ya descargada. Solo con Play Dolby cerrado. Respalda la versión
#             actual, conserva domo.json, abre la nueva y, si no arranca, regresa a la anterior.
# Sin internet o sin Releases no es un error: lo anota y termina.
# Bitácora: %LOCALAPPDATA%\PlanetarioBridge\actualizacion\actualizacion.log
param(
    [switch]$Revisar, [switch]$Instalar, [switch]$DesdeApp, [switch]$Reabrir, [switch]$Silencioso,
    [string]$Destino = $PSScriptRoot,
    [string]$Datos = (Join-Path $env:LOCALAPPDATA 'PlanetarioBridge\actualizacion'),
    [string]$ApiJson,        # pruebas: archivo local con la respuesta de la API de GitHub
    [switch]$SinArrancar     # pruebas: no abre Play Dolby al terminar
)
$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
New-Item -ItemType Directory -Force -Path $Datos | Out-Null
$bitacora = Join-Path $Datos 'actualizacion.log'
$estadoRuta = Join-Path $Datos 'estado.json'

function Log([string]$t) {
    Add-Content -LiteralPath $bitacora -Value ((Get-Date -Format 'yyyy-MM-dd HH:mm:ss') + ' ' + $t) -Encoding UTF8
    if (-not $Silencioso -and -not $DesdeApp) { Write-Host ('  ' + $t) }
}
function Ver($s) { if (-not $s) { return $null }; $s = ([string]$s).Trim().TrimStart('v', 'V'); try { return [version]$s } catch { return $null } }
function Lee([string]$ruta) { if (Test-Path -LiteralPath $ruta) { return (Get-Content -LiteralPath $ruta -Raw -Encoding UTF8 | ConvertFrom-Json) }; return $null }
function Guarda($obj) { $obj | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $estadoRuta -Encoding UTF8 }
# Resultado de la última revisión, para que Play Dolby lo muestre (botón «Buscar actualizaciones»).
function Resultado([string]$r, [string]$mensaje) {
    @{ fecha = (Get-Date -Format s); resultado = $r; mensaje = $mensaje } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Datos 'ultima.json') -Encoding UTF8
}
function Trae([string]$url, [string]$salida) {
    if ($url -match '^https?://') { Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $salida -Headers @{ 'User-Agent' = 'PlayDolby-actualizador' } -TimeoutSec 300 }
    else { Copy-Item -LiteralPath $url -Destination $salida -Force }   # pruebas con archivos locales
}
function Dialogo([string]$texto, [bool]$siNo) {
    if ($Silencioso) { return $false }
    Add-Type -AssemblyName System.Windows.Forms
    $f = New-Object System.Windows.Forms.Form; $f.TopMost = $true
    $b = if ($siNo) { [System.Windows.Forms.MessageBoxButtons]::YesNo } else { [System.Windows.Forms.MessageBoxButtons]::OK }
    $r = [System.Windows.Forms.MessageBox]::Show($f, $texto, 'Play Dolby · actualización', $b, [System.Windows.Forms.MessageBoxIcon]::Information)
    $f.Dispose()
    return $r -eq [System.Windows.Forms.DialogResult]::Yes
}
function Abiertos { Get-Process PlayDolby -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($Destino, [StringComparison]::OrdinalIgnoreCase) } }

$config = Lee (Join-Path $Destino 'actualizacion.json')
if (-not $config -or -not $config.repositorio) { Log 'Sin actualizacion.json: actualizaciones desactivadas.'; exit 0 }
$verTxt = Join-Path $Destino 'VERSION.txt'
$actual = if (Test-Path -LiteralPath $verTxt) { Ver (Get-Content -LiteralPath $verTxt -TotalCount 1) } else { $null }

function InstalarDescargada {
    $est = Lee $estadoRuta
    if (-not $est -or -not $est.disponible -or -not (Test-Path -LiteralPath (Join-Path $est.carpeta 'PlayDolby.exe'))) { Log 'No hay una actualización descargada.'; return 0 }
    for ($i = 0; $i -lt 60 -and (Abiertos); $i++) { Start-Sleep -Seconds 1 }   # Play Dolby se está cerrando
    if (Abiertos) { Log 'Play Dolby sigue abierto: no se instaló.'; [void](Dialogo 'Cierra Play Dolby (deteniendo el audio) y vuelve a intentar la actualización.' $false); return 1 }

    $respaldo = Join-Path $Datos 'respaldo'
    if (Test-Path -LiteralPath $respaldo) { Remove-Item -LiteralPath $respaldo -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $respaldo | Out-Null
    Get-ChildItem -LiteralPath $Destino | Where-Object { $_.Name -ne 'demo' } | Copy-Item -Destination $respaldo -Recurse -Force
    Log "Respaldo de la versión $actual en $respaldo"

    $conservar = Test-Path -LiteralPath (Join-Path $Destino 'domo.json')
    Get-ChildItem -LiteralPath $est.carpeta | Where-Object { -not ($_.Name -eq 'domo.json' -and $conservar) } | Copy-Item -Destination $Destino -Recurse -Force
    Get-ChildItem -LiteralPath $Destino -Recurse -File | Where-Object { $_.FullName -notlike '*\demo\*' } | Unblock-File
    $clave = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\PlayDolbyPLI'
    if (Test-Path $clave) { Set-ItemProperty -Path $clave -Name DisplayVersion -Value ([string]$est.version) }

    if (-not $SinArrancar) {
        $ok = $false
        try {
            $p = Start-Process -FilePath (Join-Path $Destino 'PlayDolby.exe') -WorkingDirectory $Destino -PassThru
            # Hasta 60 s: al arrancar, Play Dolby revisa los dispositivos de audio y puede tardar.
            for ($i = 0; $i -lt 120; $i++) { Start-Sleep -Milliseconds 500; $p.Refresh(); if ($p.HasExited) { break }; if ($p.MainWindowHandle -ne 0) { $ok = $true; break } }
        } catch { Log ('No arrancó: ' + $_.Exception.Message) }
        if (-not $ok) {
            if ($p -and -not $p.HasExited) { $p.Kill() }
            Get-ChildItem -LiteralPath $respaldo | Copy-Item -Destination $Destino -Recurse -Force
            if (Test-Path $clave) { Set-ItemProperty -Path $clave -Name DisplayVersion -Value ([string]$actual) }
            Log "La versión $($est.version) no arrancó: se regresó a $actual."
            [void](Dialogo "La versión $($est.version) no arrancó. Play Dolby regresó a la versión $actual." $false)
            Start-Process -FilePath (Join-Path $Destino 'PlayDolby.exe') -WorkingDirectory $Destino
            return 1
        }
    }
    Guarda @{ version = [string]$est.version; disponible = $false; instalado = (Get-Date -Format s) }
    Log "Actualizado a $($est.version)."
    return 0
}

if ($Revisar) {
    $api = "https://api.github.com/repos/$($config.repositorio)/releases/latest"
    try {
        $rel = if ($ApiJson) { Get-Content -LiteralPath $ApiJson -Raw | ConvertFrom-Json }
               else { Invoke-RestMethod -Uri $api -Headers @{ 'User-Agent' = 'PlayDolby-actualizador'; 'Accept' = 'application/vnd.github+json' } -TimeoutSec 30 }
    } catch {
        $m = $_.Exception.Message
        if ($m -match '404') { Log 'GitHub respondió que aún no hay versiones publicadas (Releases).'; Resultado 'aldia' "Estás al día: versión $actual. Todavía no hay versiones publicadas en GitHub." }
        else { Log ("No se pudo consultar GitHub (¿sin internet?): " + $m); Resultado 'error' 'No se pudo consultar GitHub: revisa la conexión a internet. Se vuelve a intentar en la siguiente revisión.' }
        exit 0
    }
    $nueva = Ver $rel.tag_name
    if (-not $nueva -or -not $actual -or $nueva -le $actual) {
        Log "Al día: versión $actual (último Release: $($rel.tag_name))."
        Guarda @{ version = [string]$actual; disponible = $false; revisado = (Get-Date -Format s) }
        Resultado 'aldia' "Estás al día: versión $actual."
        exit 0
    }
    $zipA = $rel.assets | Where-Object { $_.name -like 'PlayDolby-*.zip' } | Select-Object -First 1
    $shaA = $rel.assets | Where-Object { $_.name -like 'PlayDolby-*.zip.sha256' } | Select-Object -First 1
    if (-not $zipA -or -not $shaA) { Log "El Release $($rel.tag_name) no trae PlayDolby-*.zip y su .sha256: se ignora."; Resultado 'error' "La versión $nueva en GitHub está incompleta (le falta el ZIP o su huella). Se ignora."; exit 0 }
    $est = Lee $estadoRuta
    if ($est -and $est.disponible -and $est.version -eq [string]$nueva -and (Test-Path -LiteralPath (Join-Path $est.carpeta 'PlayDolby.exe'))) {
        Log "La versión $nueva ya estaba descargada."
    } else {
        $dir = Join-Path $Datos ([string]$nueva)
        if (Test-Path -LiteralPath $dir) { Remove-Item -LiteralPath $dir -Recurse -Force }
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        $zip = Join-Path $dir $zipA.name; $sha = Join-Path $dir $shaA.name
        Log "Descargando $($zipA.name)…"
        Trae $zipA.browser_download_url $zip
        Trae $shaA.browser_download_url $sha
        $esperada = (((Get-Content -LiteralPath $sha -Raw).Trim()) -split '\s+')[0].ToLower()
        $real = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLower()
        if ($esperada -ne $real) { Remove-Item -LiteralPath $dir -Recurse -Force; Log "SHA-256 NO coincide (esperada $esperada, real $real): se descartó la descarga."; Resultado 'error' "La descarga de la versión $nueva no pasó la verificación de huella (SHA-256) y se descartó."; exit 1 }
        $carpeta = Join-Path $dir 'programa'
        Expand-Archive -LiteralPath $zip -DestinationPath $carpeta -Force
        if (-not (Test-Path -LiteralPath (Join-Path $carpeta 'PlayDolby.exe'))) { Log 'El ZIP no trae PlayDolby.exe: se ignora.'; Resultado 'error' "La versión $nueva no trae PlayDolby.exe. Se ignora."; exit 1 }
        Guarda @{ version = [string]$nueva; disponible = $true; carpeta = $carpeta; notas = [string]$rel.body; actual = [string]$actual; revisado = (Get-Date -Format s) }
        Log "Versión $nueva descargada y verificada (SHA-256)."
    }
    Resultado 'nueva' "Hay una versión nueva: $nueva (tienes $actual), descargada y verificada."
    if ($DesdeApp -or (Abiertos)) { exit 0 }   # Play Dolby avisa en su ventana
    if ($Silencioso) { Log 'Modo silencioso: queda lista para instalar (no se pregunta).'; exit 0 }
    if (Dialogo "Hay una versión nueva de Play Dolby: $nueva (esta PC tiene $actual).`n`n¿Instalarla ahora?" $true) { exit (InstalarDescargada) }
    Log 'Instalación pospuesta por el usuario.'
    exit 0
}
if ($Instalar) { exit (InstalarDescargada) }
Write-Host 'Uso: actualizador.ps1 -Revisar | -Instalar'
