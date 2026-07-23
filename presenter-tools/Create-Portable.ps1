[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo',
    [string] $OutputDirectory = 'portable'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$RunDirectory = Join-Path $ProjectRoot "build_x64/rundir/$Configuration"
$SourceExecutable = Join-Path $RunDirectory 'bin/64bit/obs64.exe'
$Destination = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
    [System.IO.Path]::GetFullPath($OutputDirectory)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $ProjectRoot $OutputDirectory))
}

if (-not (Test-Path -LiteralPath $SourceExecutable)) {
    throw "No existe $SourceExecutable. Ejecuta primero Build-Presenter.ps1."
}

New-Item -ItemType Directory -Path $Destination -Force | Out-Null
Copy-Item -Path (Join-Path $RunDirectory '*') -Destination $Destination -Recurse -Force

$PortableBin = Join-Path $Destination 'bin/64bit'
$PortableExecutable = Join-Path $PortableBin 'Presentador.exe'
$CopiedObsExecutable = Join-Path $PortableBin 'obs64.exe'
if (Test-Path -LiteralPath $PortableExecutable) {
    Remove-Item -LiteralPath $PortableExecutable -Force
}
Rename-Item -LiteralPath $CopiedObsExecutable -NewName 'Presentador.exe'

$SourceBibleDirectory = Join-Path $ProjectRoot 'dist/PresentadorMultimedia/config/obs-studio/bibles'
$PortableBibleDirectory = Join-Path $Destination 'config/obs-studio/bibles'
if (Test-Path -LiteralPath $SourceBibleDirectory) {
    New-Item -ItemType Directory -Path $PortableBibleDirectory -Force | Out-Null
    Copy-Item -Path (Join-Path $SourceBibleDirectory '*.txt') -Destination $PortableBibleDirectory -Force
}

$Launcher = @'
@echo off
cd /d "%~dp0bin\64bit"
start "Presentador" Presentador.exe --portable
'@
Set-Content -LiteralPath (Join-Path $Destination 'INICIAR_PRESENTADOR.bat') -Value $Launcher -Encoding ASCII
Set-Content -LiteralPath (Join-Path $Destination 'portable_mode.txt') -Value 'Presentador portable' -Encoding ASCII

$Readme = @'
PRESENTADOR MULTIMEDIA

1. Abre INICIAR_PRESENTADOR.bat.
2. También puedes abrir bin\64bit\Presentador.exe directamente.
3. portable_mode.txt hace que preferencias y biblias se guarden dentro de esta carpeta incluso al abrir el EXE.
4. Los archivos multimedia importados no se copian; deben seguir disponibles en sus rutas originales.
'@
Set-Content -LiteralPath (Join-Path $Destination 'LEEME.txt') -Value $Readme -Encoding UTF8

Write-Host "Presentador portátil creado en: $Destination"
Write-Host "Ejecutable: $PortableExecutable"
