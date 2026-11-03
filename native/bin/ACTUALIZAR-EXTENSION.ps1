param([string]$CarpetaInstalada,[switch]$SinDialogos,[string]$CarpetaBase=$PSScriptRoot)
$ErrorActionPreference='Stop'
function Aviso([string]$texto){
  Write-Host $texto
  if(-not $SinDialogos){
    Add-Type -AssemblyName System.Windows.Forms
    [void][System.Windows.Forms.MessageBox]::Show($texto,'PLI Espacial')
  }
}
$origen=Join-Path $PSScriptRoot 'chrome-extension'
if(-not(Test-Path -LiteralPath (Join-Path $origen 'manifest.json'))){$origen=[System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\chrome-extension'))}
if(-not(Test-Path -LiteralPath (Join-Path $origen 'manifest.json'))){throw 'No encuentro los archivos actualizados de la extensión.'}
if(-not $CarpetaInstalada){
  $CarpetaInstalada=Join-Path $env:LOCALAPPDATA 'PLI\PLI-Espacial-7.1'
  if(-not(Test-Path -LiteralPath (Join-Path $CarpetaInstalada 'manifest.json'))){
    if($SinDialogos){throw 'Indica -CarpetaInstalada para actualizar una instalación existente.'}
    & (Join-Path $origen 'INSTALAR-EXTENSION.bat')
    exit $LASTEXITCODE
  }
}
$destino=[System.IO.Path]::GetFullPath($CarpetaInstalada)
$manifest=Join-Path $destino 'manifest.json'
if(-not(Test-Path -LiteralPath $manifest)){throw 'Esta carpeta no contiene una extensión instalada.'}
$previo=Get-Content -LiteralPath $manifest -Raw -Encoding UTF8 | ConvertFrom-Json
if($previo.name -ne 'PLI Espacial 7.1'){throw 'La carpeta seleccionada no es PLI Espacial 7.1.'}
$origenCompleto=[System.IO.Path]::GetFullPath($origen).TrimEnd('\')
if($origenCompleto.Equals($destino.TrimEnd('\'),[StringComparison]::OrdinalIgnoreCase)){
  Aviso "Extensión encontrada automáticamente: $destino`nVersión: $($previo.version). Esta es la extensión incluida en el paquete; sus archivos ya están en esta carpeta.`nAbre chrome://extensions y pulsa Recargar en PLI Espacial. Si Chrome usa otra carpeta, indica esa ruta con -CarpetaInstalada."
  exit 0
}
if($destino.StartsWith($origenCompleto+'\',[StringComparison]::OrdinalIgnoreCase) -or $origenCompleto.StartsWith($destino.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Las carpetas de origen y destino no pueden estar una dentro de la otra.'}
$respaldo=Join-Path (Split-Path $destino -Parent) ('PLI-respaldo-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
Copy-Item -LiteralPath $destino -Destination $respaldo -Recurse
$nuevo=Get-Content -LiteralPath (Join-Path $origen 'manifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json
Get-ChildItem -LiteralPath $origen | Where-Object {$_.Name -notin @('pruebas','manifest.json','INSTALAR-EXTENSION.bat')} | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $destino -Recurse -Force}
$nuevo | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $manifest -Encoding UTF8
Write-Host "Actualizada en: $destino"
Write-Host "Respaldo: $respaldo"
Aviso "Carpeta: $destino. Archivos actualizados. En chrome://extensions desactiva la copia anterior y carga esta carpeta con Cargar descomprimida. Activa la captura en la pestaña de música."
