# Diagnóstico de audio 7.1 de esta PC. SOLO LECTURA: no cambia nada del sistema.
# Uso: pwsh -NoProfile -ExecutionPolicy Bypass -File native\scripts\diagnostico-audio.ps1
# Responde, en orden: ¿qué anuncia el receptor por HDMI (EDID)?, ¿cómo tiene Windows configurada
# cada salida?, ¿hay mejoradores virtuales (FxSound) estorbando?, ¿qué ven PortAudio y ASIO?
# Criterio verificado el 2026-10-19 en el domo (Integra DRX-8.4 por HDMI de GPU AMD).

$ErrorActionPreference = 'Continue'
$names = @{1='LPCM';2='AC-3';7='DTS';10='E-AC-3';11='DTS-HD';12='TrueHD'}

Write-Host "== 1. EDID: lo que anuncia cada pantalla/receptor HDMI (incluye los ya desconectados: Windows los recuerda)" -ForegroundColor Cyan
Get-ChildItem 'HKLM:\SYSTEM\CurrentControlSet\Enum\DISPLAY' -ErrorAction SilentlyContinue | ForEach-Object { Get-ChildItem $_.PSPath } | ForEach-Object {
    $e = (Get-ItemProperty "$($_.PSPath)\Device Parameters" -ErrorAction SilentlyContinue).EDID
    if (-not $e -or $e.Length -lt 256) { return }
    $name = ''; for ($d = 54; $d -le 108; $d += 18) { if ($e[$d] -eq 0 -and $e[$d+3] -eq 0xFC) { $name = [Text.Encoding]::ASCII.GetString($e, $d+5, 13).Trim() } }
    $lines = @()
    for ($b = 128; $b -lt $e.Length; $b += 128) { if ($e[$b] -ne 2) { continue }; $i = $b + 4
        while ($i -lt $b + $e[$b+2]) { $tag = $e[$i] -shr 5; $len = $e[$i] -band 31
            if ($tag -eq 1) { for ($k = 0; $k -lt $len; $k += 3) { $s = $e[$i+1+$k]; $f = ($s -shr 3) -band 15; $lines += "   {0,-8} hasta {1} canales" -f $names[[int]$f], (($s -band 7) + 1) } }
            $i += $len + 1 } }
    if ($lines) { $max = ($lines | Where-Object { $_ -match 'LPCM' } | ForEach-Object { [int]($_ -replace '.*hasta (\d+).*','$1') } | Measure-Object -Maximum).Maximum
        Write-Host ("-- {0}: LPCM máximo {1} canales {2}" -f $name, $max, ($(if ($max -ge 8) { '(sirve para 7.1)' } else { '' })))
        $lines | Select-Object -Unique | ForEach-Object { Write-Host $_ } }
}

Write-Host "`n== 2. Salidas de Windows activas y su formato" -ForegroundColor Cyan
$base = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render'
Get-ChildItem $base | ForEach-Object {
    if ((Get-ItemProperty $_.PSPath).DeviceState -ne 1) { return }
    $p = Get-ItemProperty "$($_.PSPath)\Properties"
    $n = $p.'{a45c254e-df1c-4efd-8020-67d146a850e0},2'; $fmt = $p.'{f19f064d-082c-4e27-bc73-6882a1bb8e4c},0'
    if (-not $fmt) { return }
    $ch = [BitConverter]::ToUInt16($fmt, 10); $hz = [BitConverter]::ToUInt32($fmt, 12); $bits = [BitConverter]::ToUInt16($fmt, 22)
    $mask = if ($fmt.Length -ge 32) { [BitConverter]::ToUInt32($fmt, 28) } else { 0 }
    $ok = if ($ch -eq 8 -and $mask -eq 0x63F) { '7.1 OK' } elseif ($ch -eq 2) { 'ESTÉREO' } else { '' }
    Write-Host ("   {0,-28} {1} canales  {2} Hz  {3} bit  máscara 0x{4:X}  {5}" -f $n, $ch, $hz, $bits, $mask, $ok)
    Write-Host ("      id: {{0.0.0.00000000}}.{0}" -f $_.PSChildName.ToLower())
}
Write-Host "   Si el receptor sale ESTÉREO: mmsys.cpl > Reproducción > la salida HDMI > Configurar > 7.1 envolvente."

Write-Host "`n== 3. Mejoradores virtuales (fuerzan estéreo si se interponen)" -ForegroundColor Cyan
$fx = Get-Process | Where-Object { $_.ProcessName -match 'fxsound|dfx|nahimic|sonic' }
if ($fx) { $fx | ForEach-Object { Write-Host "   ABIERTO: $($_.ProcessName) — ciérralo antes de probar 7.1" -ForegroundColor Yellow } } else { Write-Host "   Ninguno abierto." }

Write-Host "`n== 4. PortAudio / ASIO (consola Qu y salida HDMI)" -ForegroundColor Cyan
$hw = Join-Path $PSScriptRoot 'hw_bridge_test.exe'                       # versión instalada (misma carpeta)
if (-not (Test-Path $hw)) { $hw = Join-Path $PSScriptRoot '..\bin\hw_bridge_test.exe' }  # dentro del proyecto
if (Test-Path $hw) { & $hw --list } else { Write-Host "   Falta native\bin\hw_bridge_test.exe (se compila con native\build-cinematic.ps1)." }
