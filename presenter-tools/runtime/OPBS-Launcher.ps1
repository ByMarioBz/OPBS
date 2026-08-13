[CmdletBinding()]
param([switch] $SafeMode)

$ErrorActionPreference = 'Stop'
$Updater = Join-Path $PSScriptRoot 'OPBS-Updater.ps1'
$Executable = Join-Path $PSScriptRoot 'OPBS.exe'

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
