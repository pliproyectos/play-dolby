# Arma el paquete portable/instalable de Play Dolby (por ejemplo en la USB).
#   pwsh -File native\empaquetar-usb.ps1 -Destino "D:\PLAY DOLBY"
# Usa los ejecutables ya compilados de native\bin (compilar antes con build-cinematic/espacial/iamf).
# Resultado:
#   INSTALAR.cmd · ABRIR SIN INSTALAR.cmd · LEEME.txt
#   instalador\  (instalar.ps1, desinstalar.ps1)
#   programa\    (PlayDolby.exe, portaudio.dll, domo.json, herramientas\…)
#   demo\        (capas de Cosmic para «Carpeta de capas…» y prueba-7.1.wav)
#   -Reemplazar: borra antes un paquete ANTERIOR de Play Dolby en ese destino (solo si lo es).
#   -Version     : versión del paquete (por omisión, la fecha: 2026.10.22). Para una segunda versión el mismo día: 2026.10.22.1
#   -Repositorio : de dónde se actualizan las terminales (Releases públicos de GitHub)
param([Parameter(Mandatory)][string]$Destino, [switch]$SinDemo, [switch]$Reemplazar,
      [string]$Version = (Get-Date -Format 'yyyy.MM.dd'), [string]$Repositorio = 'pliproyectos/play-dolby')
$ErrorActionPreference = 'Stop'
$proy = Split-Path $PSScriptRoot -Parent
$bin = Join-Path $PSScriptRoot 'bin'
$fuente = Join-Path $proy 'instalador'
if (Test-Path -LiteralPath $Destino) {
    $esPaquete = (Test-Path -LiteralPath (Join-Path $Destino 'INSTALAR.cmd')) -and (Test-Path -LiteralPath (Join-Path $Destino 'programa\PlayDolby.exe'))
    if (-not $Reemplazar) { throw "Ya existe $Destino. Usa -Reemplazar si es un paquete anterior de Play Dolby." }
    if (-not $esPaquete) { throw "$Destino existe y NO parece un paquete de Play Dolby (falta INSTALAR.cmd o programa\PlayDolby.exe). No se borra." }
    Remove-Item -LiteralPath $Destino -Recurse -Force
    "Paquete anterior borrado: $Destino"
}

# Windows PowerShell 5.1 (el de fábrica) lee .ps1 sin BOM como ANSI: se escriben UTF-8 con BOM.
function CopiaPs1([string]$de, [string]$a) { [IO.File]::WriteAllText($a, [IO.File]::ReadAllText($de), (New-Object Text.UTF8Encoding $true)) }
# .cmd/.txt con fin de línea CRLF.
function CopiaTexto([string]$de, [string]$a) { [IO.File]::WriteAllText($a, ([IO.File]::ReadAllText($de) -replace "`r?`n", "`r`n"), (New-Object Text.UTF8Encoding $false)) }

$prog = Join-Path $Destino 'programa'; $her = Join-Path $prog 'herramientas'; $ins = Join-Path $Destino 'instalador'
New-Item -ItemType Directory -Force -Path $prog, $her, $ins, (Join-Path $prog 'assets') | Out-Null

CopiaTexto (Join-Path $fuente 'INSTALAR.cmd') (Join-Path $Destino 'INSTALAR.cmd')
CopiaTexto (Join-Path $fuente 'ABRIR SIN INSTALAR.cmd') (Join-Path $Destino 'ABRIR SIN INSTALAR.cmd')
CopiaTexto (Join-Path $fuente 'LEEME.txt') (Join-Path $Destino 'LEEME.txt')
CopiaTexto (Join-Path $fuente 'LEEME.txt') (Join-Path $prog 'LEEME.txt')
CopiaPs1 (Join-Path $fuente 'instalar.ps1') (Join-Path $ins 'instalar.ps1')
CopiaPs1 (Join-Path $fuente 'desinstalar.ps1') (Join-Path $ins 'desinstalar.ps1')

foreach ($f in 'PlayDolby.exe', 'portaudio.dll') { Copy-Item (Join-Path $bin $f) $prog }
Copy-Item (Join-Path $PSScriptRoot 'assets\pli-logo.png') (Join-Path $prog 'assets')
Copy-Item (Join-Path $proy 'pruebas-domo\domo.json') $prog
foreach ($f in 'escena71.exe', 'adm71.exe', 'capas71.exe', 'upmix71.exe', 'iamf71.exe', 'iamfdec.exe', 'hw_bridge_test.exe') { Copy-Item (Join-Path $bin $f) $her }
Copy-Item (Join-Path $bin 'portaudio.dll') $her   # hw_bridge_test la necesita junto
CopiaPs1 (Join-Path $PSScriptRoot 'scripts\diagnostico-audio.ps1') (Join-Path $her 'diagnostico-audio.ps1')
CopiaPs1 (Join-Path $PSScriptRoot 'scripts\check-qu5-driver.ps1') (Join-Path $her 'check-qu5-driver.ps1')
CopiaTexto (Join-Path $fuente 'prueba-hardware.cmd') (Join-Path $her 'prueba-hardware.cmd')
Copy-Item (Join-Path $PSScriptRoot 'espacial\ESCENAS.md') $her
# Guía de usuario con sus capturas.
$guia = Join-Path $prog 'guia'; New-Item -ItemType Directory -Force -Path (Join-Path $guia 'img') | Out-Null
Copy-Item (Join-Path $proy 'docs\GUIA-DE-USUARIO.md') $guia
Copy-Item (Join-Path $proy 'docs\img\*.png') (Join-Path $guia 'img')
Copy-Item (Join-Path $proy 'docs\GUIA-DE-USUARIO.md') (Join-Path $Destino 'GUIA-DE-USUARIO.md')

try { [void][version]$Version } catch { throw "Versión inválida: $Version (formato 2026.10.22 o 2026.10.22.1)" }
$version = $Version
Set-Content -LiteralPath (Join-Path $prog 'VERSION.txt') -Value @($version, "Compilado: $((Get-Item (Join-Path $bin 'PlayDolby.exe')).LastWriteTime.ToString('yyyy-MM-dd HH:mm'))") -Encoding ascii
# Actualizaciones con aviso desde los Releases de GitHub (instalador\actualizador.ps1).
CopiaPs1 (Join-Path $fuente 'actualizador.ps1') (Join-Path $prog 'actualizador.ps1')
[ordered]@{ repositorio = $Repositorio; intervalo_horas = 12; modo = 'con aviso' } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $prog 'actualizacion.json') -Encoding ascii

if (-not $SinDemo) {
    $demo = Join-Path $Destino 'demo'
    $capas = Join-Path $demo 'capas-cosmic'; $musica = Join-Path $demo 'musica-7.1'; $pruebas = Join-Path $demo 'pruebas-7.1'
    New-Item -ItemType Directory -Force -Path $capas, $musica, $pruebas | Out-Null
    # Capas para «▶ Carpeta de capas…» (PLI Espacial en vivo, sin consola).
    Copy-Item (Join-Path $proy 'musica-7.1\pli-espacial-cosmic\capas\*.wav') $capas
    # Música ya mezclada en 7.1 (para «▶ Archivo WAV…» o VLC).
    $m = Join-Path $proy 'musica-7.1'
    $canciones = [ordered]@{
        'pli-espacial-cosmic\Cosmic Home Theater Experience - 7.1-instrumentos.wav'          = 'Cosmic - PLI Espacial (instrumentos que se mueven).wav'
        'pli-espacial-cosmic\Cosmic Home Theater Experience - estable - 7.1-escena.wav'     = 'Cosmic - PLI Espacial estable (sin aceleron).wav'
        'pli-espacial\Cosmic Home Theater Experience (Remix) - 7.1-instrumentos.wav'        = 'Cosmic Remix - PLI Espacial.wav'
        'Cosmic Home Theater Experience (Remix) - 7.1-orbita.wav'                           = 'Cosmic Remix - orbita (toda la mezcla gira).wav'
        'Cosmic Home Theater Experience (Remix) - 7.1-conservador.wav'                      = 'Cosmic Remix - conservador.wav'
        'Cosmic Home Theater Experience (Remix) - 7.1-instrumentos.wav'                     = 'Cosmic Remix - instrumentos (anillo clasico).wav'
    }
    foreach ($k in $canciones.Keys) { Copy-Item -LiteralPath (Join-Path $m $k) -Destination (Join-Path $musica $canciones[$k]) }
    # Señales de prueba 7.1 del kit del domo (48 kHz) y su guía.
    Copy-Item (Join-Path $proy 'prueba-7.1.wav') (Join-Path $pruebas '00-tonos por bocina.wav')
    $kit = Join-Path $proy 'pruebas-domo\salida\48kHz'
    foreach ($f in Get-ChildItem -LiteralPath $kit -Filter *.wav) {
        $n = $f.Name; if ($n -like '04-calibracion*') { $n = '04-calibracion (FUERTE, -20 dBFS - para sonometro).wav' }
        Copy-Item -LiteralPath $f.FullName -Destination (Join-Path $pruebas $n)
    }
    Copy-Item (Join-Path $proy 'pruebas-domo\LEEME.md') (Join-Path $pruebas 'LEEME-pruebas.md')
    Copy-Item (Join-Path $proy 'pruebas-domo\hoja-de-campo.html') $pruebas
    Copy-Item (Join-Path $proy 'pruebas-domo\salida\GUION.md') $pruebas
}
$f = Get-ChildItem -LiteralPath $Destino -Recurse -File
"Paquete listo en $Destino · versión $version · {0} archivos · {1:N0} MB" -f $f.Count, (($f | Measure-Object Length -Sum).Sum / 1MB)
