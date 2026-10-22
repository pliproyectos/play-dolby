# Prepara una versión nueva de Play Dolby para que las terminales se actualicen (con aviso).
#   pwsh -File native\publicar-version.ps1 -Version 2026.10.25
# Antes: compilar con los tres build-*.ps1 (corren las pruebas) y hacer commit + push del código.
# Resultado en publicar\ (no se versiona):
#   PlayDolby-<versión>.zip         el programa (sin demo); es lo que bajan las terminales
#   PlayDolby-<versión>.zip.sha256  su huella; las terminales rechazan el ZIP si no coincide
# Después: crear el Release en GitHub con la etiqueta v<versión> y subir esos DOS archivos. Este script
# abre la página. Al publicarlo, cada terminal lo verá al prender la PC o en ≤ 12 h con Play Dolby abierto.
param([Parameter(Mandatory)][string]$Version, [string]$Repositorio = 'pliproyectos/play-dolby', [switch]$NoAbrir)
$ErrorActionPreference = 'Stop'
try { [void][version]$Version } catch { throw "Versión inválida: $Version (formato 2026.10.25 o 2026.10.25.1)" }
$proy = Split-Path $PSScriptRoot -Parent
$salida = Join-Path $proy 'publicar'
$paquete = Join-Path $salida "paquete-$Version"
$zip = Join-Path $salida "PlayDolby-$Version.zip"
New-Item -ItemType Directory -Force -Path $salida | Out-Null
if (Test-Path -LiteralPath $zip) { throw "Ya existe $zip. Usa otra versión (por ejemplo $Version.1)." }

& (Join-Path $PSScriptRoot 'empaquetar-usb.ps1') -Destino $paquete -SinDemo -Version $Version -Repositorio $Repositorio -Reemplazar
Compress-Archive -Path (Join-Path $paquete 'programa\*') -DestinationPath $zip
$sha = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLower()
Set-Content -LiteralPath "$zip.sha256" -Value "$sha  PlayDolby-$Version.zip" -Encoding ascii

Write-Host ''
Write-Host "Listo para publicar Play Dolby $Version" -ForegroundColor Green
Write-Host "  $zip  ($([math]::Round((Get-Item $zip).Length / 1MB, 1)) MB)"
Write-Host "  $zip.sha256  ($sha)"
Write-Host ''
Write-Host 'En GitHub (se abre la página):'
Write-Host "  1. Etiqueta (tag): v$Version   · Título: Play Dolby $Version"
Write-Host '  2. Describe los cambios (las terminales lo guardan como notas).'
Write-Host '  3. Arrastra los DOS archivos de arriba a «Attach binaries».'
Write-Host '  4. «Publish release». Eso es la señal para todas las terminales.'
if (-not $NoAbrir) { Start-Process "https://github.com/$Repositorio/releases/new?tag=v$Version&title=Play%20Dolby%20$Version" }
