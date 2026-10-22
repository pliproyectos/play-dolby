@echo off
rem Prueba fisica Qu-5 (ASIO) -> Integra (HDMI 7.1) en SILENCIO digital, 2 minutos.
rem Cerrar Play Dolby antes: el HDMI se abre en modo exclusivo.
cd /d "%~dp0native\bin"
hw_bridge_test.exe --list
echo.
hw_bridge_test.exe --run 120 --exclusive
echo.
if errorlevel 1 (echo RESULTADO: FALLA) else (echo RESULTADO: PASA)
pause
