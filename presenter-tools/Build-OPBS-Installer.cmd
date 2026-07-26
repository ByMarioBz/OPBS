@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-OPBS-Installer.ps1" %*
exit /b %errorlevel%
