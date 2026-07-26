@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Ensure-OPBS-InstallerTools.ps1" %*
exit /b %errorlevel%
