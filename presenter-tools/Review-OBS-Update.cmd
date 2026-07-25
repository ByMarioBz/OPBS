@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Review-OBS-Update.ps1" %*
exit /b %errorlevel%
