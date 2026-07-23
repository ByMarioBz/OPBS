@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Create-Portable.ps1" %*
exit /b %errorlevel%
