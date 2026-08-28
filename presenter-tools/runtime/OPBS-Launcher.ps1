[CmdletBinding()]
param([switch] $SafeMode)

$ErrorActionPreference = 'Stop'
$Updater = Join-Path $PSScriptRoot 'OPBS-Updater.ps1'
$Executable = Join-Path $PSScriptRoot 'Presenter Broadcast Studio.exe'
if (-not (Test-Path -LiteralPath $Executable)) {
    # Compatibilidad al ejecutar el lanzador durante una actualización desde 0.1.6 o anterior.
    $Executable = Join-Path $PSScriptRoot 'OPBS.exe'
}

try {
    $Arguments = @{ LaunchApp = $true }
    if ($SafeMode) {
        $Arguments.SafeMode = $true
    }
    & $Updater @Arguments
    exit $LASTEXITCODE
} catch {
    $FallbackArguments = @('--disable-updater')
    if ($SafeMode) {
        $FallbackArguments += '--safe-mode'
    }
    Start-Process -FilePath $Executable -ArgumentList $FallbackArguments -WorkingDirectory $PSScriptRoot
    exit 0
}
