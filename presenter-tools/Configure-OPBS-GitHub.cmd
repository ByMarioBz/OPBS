@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Configure-OPBS-GitHub.ps1" %*
exit /b %errorlevel%
