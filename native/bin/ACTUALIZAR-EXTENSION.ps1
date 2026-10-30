param([string]$CarpetaInstalada,[switch]$SinDialogos)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms
$origen=Join-Path $PSScriptRoot 'chrome-extension'
if(-not(Test-Path -LiteralPath (Join-Path $origen 'manifest.json'))){$origen=[System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\chrome-extension'))}
if(-not(Test-Path -LiteralPath (Join-Path $origen 'manifest.json'))){throw 'No encuentro los archivos actualizados de la extensión.'}
if(-not $CarpetaInstalada){
  if($SinDialogos){throw 'Indica -CarpetaInstalada para una actualización sin diálogos.'}
  [void][System.Windows.Forms.MessageBox]::Show('Selecciona la carpeta de la extensión que YA aparece instalada en Chrome. Conservar esa carpeta conserva el identificador que reconoce este EXE. Después pulsa Recargar en chrome://extensions.','Actualizar PLI Espacial')
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
if($origenCompleto -eq $destino.TrimEnd('\') -or $destino.StartsWith($origenCompleto+'\',[StringComparison]::OrdinalIgnoreCase) -or $origenCompleto.StartsWith($destino.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Selecciona la carpeta instalada de Chrome, separada de esta copia de actualización.'}
$respaldo=Join-Path (Split-Path $destino -Parent) ('PLI-respaldo-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
Copy-Item -LiteralPath $destino -Destination $respaldo -Recurse
# Conserva una clave de identidad si la instalación ya la tenía.
$nuevo=Get-Content -LiteralPath (Join-Path $origen 'manifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json
if($previo.PSObject.Properties.Name -contains 'key'){$nuevo | Add-Member -NotePropertyName key -NotePropertyValue $previo.key -Force}
Get-ChildItem -LiteralPath $origen | Where-Object {$_.Name -notin @('pruebas','manifest.json','INSTALAR-EXTENSION.bat')} | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $destino -Recurse -Force}
$nuevo | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $manifest -Encoding UTF8
Write-Host "Actualizada en: $destino"
Write-Host "Respaldo: $respaldo"
if(-not $SinDialogos){[void][System.Windows.Forms.MessageBox]::Show('Archivos actualizados. Abre chrome://extensions y pulsa Recargar en PLI Espacial. Después activa la captura desde la pestaña de música.','PLI Espacial')}
