@echo off
rem Comprueba que los WAV del kit llegaron intactos y cumplen su manifiesto.
chcp 65001 >nul
cd /d "%~dp0"
where python >nul 2>nul || (echo No hay Python en esta PC. El reporte original esta en salida\VERIFICACION.md & pause & exit /b 1)
set PYTHONIOENCODING=utf-8
python verificar.py
echo.
echo Reporte completo: salida\VERIFICACION.md
pause
