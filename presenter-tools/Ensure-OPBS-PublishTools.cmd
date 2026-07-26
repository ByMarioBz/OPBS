@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Ensure-OPBS-PublishTools.ps1" %*
exit /b %errorlevel%
