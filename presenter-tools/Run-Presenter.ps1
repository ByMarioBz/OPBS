[CmdletBinding()]
param([switch] $SafeMode)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Executable = Join-Path $ProjectRoot 'dist/PresentadorMultimedia/bin/64bit/obs64.exe'

if (-not (Test-Path -LiteralPath $Executable)) {
    throw 'No existe la aplicación empaquetada. Ejecuta Package-Presenter.ps1.'
}

$Arguments = @('--portable', '--disable-updater')
if ($SafeMode) { $Arguments += '--safe-mode' }

Start-Process -FilePath $Executable -ArgumentList $Arguments -WorkingDirectory (Split-Path -Parent $Executable)
Write-Host 'Presentador multimedia iniciado.'
