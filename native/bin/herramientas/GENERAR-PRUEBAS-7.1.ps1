param([ValidateSet('48000','96000')][string]$Frecuencia='48000',[string]$Destino)
$ErrorActionPreference='Stop'
$scriptDir=Split-Path -Parent $MyInvocation.MyCommand.Path
if(-not $Destino){
  Add-Type -AssemblyName System.Windows.Forms
  $selector=New-Object System.Windows.Forms.FolderBrowserDialog
  $selector.Description='Elige dónde guardar las pruebas WAV 7.1'
  $selector.ShowNewFolderButton=$true
  if($selector.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK){exit 0}
  $Destino=$selector.SelectedPath
}
$destinoCompleto=[System.IO.Path]::GetFullPath($Destino)
if(-not(Test-Path -LiteralPath $destinoCompleto)){New-Item -ItemType Directory -Path $destinoCompleto|Out-Null}
$fecha=Get-Date -Format 'yyyyMMdd-HHmmss'
$salida=Join-Path $destinoCompleto ('Pruebas-PLI-7.1-'+$fecha)
New-Item -ItemType Directory -Path $salida|Out-Null
try{
  Add-Type -Path (Join-Path $scriptDir 'GENERAR-PRUEBAS-7.1.cs') -ErrorAction Stop
  $mensaje=[PliPruebas.Generador]::Ejecutar($salida,[int]$Frecuencia)
  $guia=@"
PRUEBAS DE AUDIO PLI 7.1 — PCM 24 bit / $Frecuencia Hz

Orden de los canales: FL, FR, FC, LFE, BL, BR, SL, SR.
Nivel máximo de las señales: -30 dBFS.
1. 01-Tonos: identifica cada salida. Un tono breve suena por una salida a la vez.
2. 02-Vuelta: tono que recorre los siete canales principales; el LFE queda en silencio.
3. 03-Estereo-frontal: L suena por FL, R por FR, el centro y el ambiente quedan en silencio.
4. 04-Centro-FC: el tono solo sale por la bocina central.
Verificación digital; la reproducción física depende del ajuste 7.1 del receptor y la app.
Volumen del receptor bajo durante las pruebas.
"@
  Set-Content -LiteralPath (Join-Path $salida 'LEEME.txt') -Value $guia -Encoding UTF8
  Write-Host $mensaje -ForegroundColor Green
  Write-Host ('Archivos guardados en: '+$salida) -ForegroundColor Green
}catch{
  Write-Error ('No se pudieron generar las señales: '+$_.Exception.Message)
  if(Test-Path -LiteralPath $salida){Get-ChildItem -LiteralPath $salida -File|Remove-Item -Force}
  throw
}
