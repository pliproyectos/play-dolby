@echo off
rem Play Dolby - instalador para este usuario (sin administrador). Doble clic.
title Instalar Play Dolby
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0instalador\instalar.ps1"
