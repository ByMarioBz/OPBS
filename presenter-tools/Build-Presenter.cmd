@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-Presenter.ps1" %*
exit /b %errorlevel%
