@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Package-Presenter.ps1" %*
exit /b %errorlevel%
