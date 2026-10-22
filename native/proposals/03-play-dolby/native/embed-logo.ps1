$ErrorActionPreference='Stop'
$logoPath=Join-Path $PSScriptRoot '..\LOGO\Logotipos PLI Horizontal RGB_Mesa de trabajo 1.png'
$bytes=[System.IO.File]::ReadAllBytes($logoPath)
$values=($bytes | ForEach-Object { '0x{0:X2}' -f $_ }) -join ','
$header="#pragma once`n// Generated from the company logo in LOGO/ by embed-logo.ps1.`nstatic const unsigned char pliLogoBytes[] = {$values};`n"
[System.IO.File]::WriteAllText((Join-Path $PSScriptRoot 'src\pli_logo_data.hpp'),$header)
