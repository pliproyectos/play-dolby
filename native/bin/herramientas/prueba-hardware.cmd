@echo off
rem Prueba fisica Qu-5 (ASIO) -> receptor (HDMI 7.1) en SILENCIO digital, 2 minutos.
rem Cerrar Play Dolby antes: el HDMI se abre en modo exclusivo.
title Prueba de hardware - Play Dolby
cd /d "%~dp0"
hw_bridge_test.exe --list
echo.
hw_bridge_test.exe --run 120 --exclusive
echo.
if errorlevel 1 (echo RESULTADO: FALLA) else (echo RESULTADO: PASA)
pause
