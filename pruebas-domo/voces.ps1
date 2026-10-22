# Genera las voces de identificación con la síntesis de Windows (sin internet).
# Salida: voces\<clave>.wav, mono, 48 kHz, 16 bit. Uso: powershell -File voces.ps1 [-Voz "Microsoft Sabina Desktop"]
param([string]$Voz = "Microsoft Sabina Desktop")

$aqui = Split-Path -Parent $MyInvocation.MyCommand.Path
$dest = Join-Path $aqui "voces"
New-Item -ItemType Directory -Force -Path $dest | Out-Null

$frases = [ordered]@{
  "FL" = "Frontal izquierdo";   "FR" = "Frontal derecho";   "FC" = "Central";   "LFE" = "Subgrave"
  "BL" = "Trasero izquierdo";   "BR" = "Trasero derecho";   "SL" = "Lateral izquierdo";   "SR" = "Lateral derecho"
  "MONO" = "Mono"
  "uno" = "Uno";   "dos" = "Dos"
  "vuelta" = "Vuelta al domo. Sentido horario.";   "antihorario" = "Sentido antihorario."
  "par-FL-FC" = "Frontal izquierdo y central";   "par-FC-FR" = "Central y frontal derecho"
  "par-FR-SR" = "Frontal derecho y lateral derecho";   "par-SR-BR" = "Lateral derecho y trasero derecho"
  "par-BR-BL" = "Trasero derecho y trasero izquierdo";   "par-BL-SL" = "Trasero izquierdo y lateral izquierdo"
  "par-SL-FL" = "Lateral izquierdo y frontal izquierdo"
  "ref-mono" = "Referencia mono.";   "ref-estereo" = "Referencia estereo."
  "ref-5.1" = "Referencia cinco punto uno, con traseros.";   "ref-5.1-laterales" = "Referencia cinco punto uno, con laterales."
  "ref-7.1" = "Referencia siete punto uno."
  "apertura" = "Rutina de apertura."
}

Add-Type -AssemblyName System.Speech
$s = New-Object System.Speech.Synthesis.SpeechSynthesizer
# Primero la voz pedida; si no se deja, cualquier voz es-MX y luego cualquier es-*. Nunca inglés.
$candidatas = @($Voz) + ($s.GetInstalledVoices() | Where-Object { $_.VoiceInfo.Culture.Name -eq "es-MX" } | ForEach-Object { $_.VoiceInfo.Name }) + ($s.GetInstalledVoices() | Where-Object { $_.VoiceInfo.Culture.Name -like "es-*" } | ForEach-Object { $_.VoiceInfo.Name })
$elegida = $null
foreach ($c in $candidatas) { try { $s.SelectVoice($c); $elegida = $c; break } catch { } }
if (-not $elegida -or $s.Voice.Culture.Name -notlike "es-*") { throw "No hay voz en español utilizable" }
$s.Rate = -1
$formato = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(48000, [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)
foreach ($k in $frases.Keys) {
  $s.SetOutputToWaveFile((Join-Path $dest "$k.wav"), $formato)
  $s.Speak($frases[$k])
}
$s.SetOutputToNull()
$s.Dispose()
"Voces generadas con '$elegida' en $dest"
