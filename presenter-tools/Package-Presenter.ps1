[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$RunDirectory = Join-Path $ProjectRoot "build_x64/rundir/$Configuration"
$Destination = Join-Path $ProjectRoot 'dist/PresentadorMultimedia'
$Executable = Join-Path $RunDirectory 'bin/64bit/obs64.exe'

if (-not (Test-Path -LiteralPath $Executable)) {
    throw "No existe $Executable. Ejecuta primero Build-Presenter.ps1."
}

New-Item -ItemType Directory -Path $Destination -Force | Out-Null
Copy-Item -Path (Join-Path $RunDirectory '*') -Destination $Destination -Recurse -Force
Set-Content -LiteralPath (Join-Path $Destination 'bin/64bit/disable_updater.txt') `
    -Value 'Las actualizaciones oficiales de OBS se integran mediante la revision protegida de Presentador.' `
    -Encoding ASCII

$Launcher = @'
@echo off
cd /d "%~dp0bin\64bit"
start "Presentador multimedia" obs64.exe --portable --disable-updater
'@
Set-Content -LiteralPath (Join-Path $Destination 'INICIAR_PRESENTADOR.bat') -Value $Launcher -Encoding ASCII

Write-Host "Aplicación portátil actualizada: $Destination"
Write-Warning 'La carpeta config conserva preferencias y rutas locales existentes; no contiene los archivos multimedia.'
