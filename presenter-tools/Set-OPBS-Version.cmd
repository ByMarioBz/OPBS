@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Set-OPBS-Version.ps1" %*
exit /b %errorlevel%
