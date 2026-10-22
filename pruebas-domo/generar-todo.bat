@echo off
rem Regenera los programas desde domo.json y los verifica. Uso: generar-todo.bat [48000] [96000]
chcp 65001 >nul
cd /d "%~dp0"
where python >nul 2>nul || (echo Hace falta Python 3.12 o mas nuevo. & pause & exit /b 1)
set PYTHONIOENCODING=utf-8
python generar.py %* || (echo El generador se detuvo. & pause & exit /b 1)
python verificar.py
pause
