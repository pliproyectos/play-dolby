@echo off
setlocal
cd /d "%~dp0"
if not exist "native\bin\PlayDolby.exe" (
    echo Recuperando Play Dolby...
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0native\build-cinematic.ps1"
    if errorlevel 1 (
        echo No se pudo reconstruir Play Dolby. Revisa el mensaje anterior.
        pause
        exit /b 1
    )
)
if not exist "native\bin\portaudio.dll" (
    echo Falta native\bin\portaudio.dll. Recuperala desde el respaldo Play Dolby.
    pause
    exit /b 1
)
echo Comprobando controlador Allen ^& Heath Qu-5...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0native\scripts\check-qu5-driver.ps1"
if errorlevel 1 (
    echo.
    echo Revisa el aviso anterior. Play Dolby puede abrirse para probar la salida HDMI.
    pause
)
start "" /D "%~dp0native\bin" "%~dp0native\bin\PlayDolby.exe"
