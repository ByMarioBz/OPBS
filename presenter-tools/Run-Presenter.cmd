@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Run-Presenter.ps1" %*
exit /b %errorlevel%
