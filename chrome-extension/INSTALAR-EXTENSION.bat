@echo off
rem PLI Espacial 7.1 - instalador de la extension de Chrome (PLI Ingenieria Aplicada).
rem Chrome no permite instalar extensiones propias sin unos clics del usuario (seguridad).
rem Este archivo hace todo lo demas: copia la extension a una carpeta fija de esta PC
rem (para que no dependa de la USB), copia la ruta al portapapeles y abre Chrome en sus extensiones.
rem Volver a correrlo actualiza los archivos (en Chrome: boton "Recargar" de la extension).
rem Uso de prueba sin abrir Chrome ni tocar el portapapeles:  INSTALAR-EXTENSION.bat /prueba <carpeta>
setlocal
chcp 65001 >nul
title Instalar extension PLI Espacial 7.1

set "ORIGEN=%~dp0"
set "DESTINO=%LOCALAPPDATA%\PLI\PLI-Espacial-7.1"
if /i "%~1"=="/prueba" set "DESTINO=%~2"

if not exist "%ORIGEN%manifest.json" (
  echo No encuentro manifest.json junto a este archivo. Corre el .bat desde la carpeta de la extension.
  pause & exit /b 1
)

echo.
echo   PLI Espacial 7.1 - extension de Chrome
echo   ======================================
echo   Copiando la extension a:
echo   %DESTINO%
robocopy "%ORIGEN%." "%DESTINO%" /E /NFL /NDL /NJH /NJS /NP /XD pruebas /XF INSTALAR-EXTENSION.bat >nul
if errorlevel 8 ( echo   ERROR al copiar. & pause & exit /b 1 )
if not exist "%DESTINO%\manifest.json" ( echo   ERROR: la copia quedo incompleta. & pause & exit /b 1 )
echo   Listo.

if /i "%~1"=="/prueba" ( echo   [prueba] sin abrir Chrome ni usar el portapapeles. & exit /b 0 )

rem Ruta al portapapeles, sin salto de linea, para pegarla en "Cargar descomprimida".
<nul set /p "=%DESTINO%" | clip

set "CHROME="
for %%C in ("%ProgramFiles%\Google\Chrome\Application\chrome.exe" "%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe" "%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe") do (
  if not defined CHROME if exist %%C set "CHROME=%%~C"
)

echo.
echo   AHORA EN CHROME (solo la primera vez):
echo     1. Arriba a la derecha, activa "Modo de desarrollador".
echo     2. Pulsa "Cargar descomprimida".
echo     3. En la ventana, pega la ruta (Ctrl+V): ya esta copiada.  Pulsa "Seleccionar carpeta".
echo     4. Fija el icono de PLI en la barra (pieza de rompecabezas, chincheta).
echo.
echo   Si ya estaba instalada: en chrome://extensions pulsa el boton de recargar de "PLI Espacial 7.1".
echo.
echo   Antes de usarla: Windows con la salida del receptor en 7.1 y predeterminada,
echo   Play Dolby cerrado y el volumen del receptor bajo.
echo.
if defined CHROME (
  start "" "%CHROME%" "chrome://extensions/"
) else (
  echo   No encontre Chrome. Abrelo tu y escribe en la barra:  chrome://extensions
)
pause
