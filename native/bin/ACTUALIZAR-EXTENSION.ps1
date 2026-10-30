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
  # Busca primero junto al BAT, tanto en la carpeta portable como en el repositorio.
  $candidatas=@((Join-Path $CarpetaBase 'chrome-extension'),(Join-Path $PSScriptRoot 'chrome-extension'),$origen)
  foreach($candidata in $candidatas){
    $rutaManifest=Join-Path $candidata 'manifest.json'
    if(Test-Path -LiteralPath $rutaManifest){
      $detectado=Get-Content -LiteralPath $rutaManifest -Raw -Encoding UTF8 | ConvertFrom-Json
      if($detectado.name -eq 'PLI Espacial 7.1'){$CarpetaInstalada=$candidata;break}
    }
  }
}
if(-not $CarpetaInstalada){
  if($SinDialogos){throw 'No encuentro chrome-extension junto al BAT. Indica -CarpetaInstalada.'}
  Add-Type -AssemblyName System.Windows.Forms
  $dialogo=New-Object System.Windows.Forms.FolderBrowserDialog
  $dialogo.Description='Carpeta actual de PLI Espacial (contiene manifest.json)'
  if($dialogo.ShowDialog() -ne 'OK'){exit 0}
  $CarpetaInstalada=$dialogo.SelectedPath
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
# Conserva una clave de identidad si la instalación ya la tenía.
$nuevo=Get-Content -LiteralPath (Join-Path $origen 'manifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json
if($previo.PSObject.Properties.Name -contains 'key'){$nuevo | Add-Member -NotePropertyName key -NotePropertyValue $previo.key -Force}
Get-ChildItem -LiteralPath $origen | Where-Object {$_.Name -notin @('pruebas','manifest.json','INSTALAR-EXTENSION.bat')} | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $destino -Recurse -Force}
$nuevo | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $manifest -Encoding UTF8
Write-Host "Actualizada en: $destino"
Write-Host "Respaldo: $respaldo"
Aviso 'Archivos actualizados. Abre chrome://extensions y pulsa Recargar en PLI Espacial. Después activa la captura desde la pestaña de música.'
