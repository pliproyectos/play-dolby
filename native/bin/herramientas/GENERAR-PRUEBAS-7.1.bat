@echo off
chcp 65001 >nul
title Generar pruebas de audio PLI 7.1
echo.
echo   GENERADOR LOCAL DE PRUEBAS PLI 7.1
echo   Crea cuatro WAV PCM de 24 bits sin necesitar internet ni descargas.
echo.
echo   1. 48 kHz (recomendado para HDMI)
echo   2. 96 kHz
echo.
set "OPCION=1"
set /p "OPCION=Elige una frecuencia [1]: "
set "FRECUENCIA=48000"
if "%OPCION%"=="2" set "FRECUENCIA=96000"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0GENERAR-PRUEBAS-7.1.ps1" -Frecuencia "%FRECUENCIA%"
if errorlevel 1 (
  echo.
  echo No se pudieron generar las pruebas. Revisa el mensaje y libera espacio si hace falta.
  pause
  exit /b 1
)
echo.
pause
