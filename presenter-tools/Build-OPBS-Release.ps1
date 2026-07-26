[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $Version,
    [string] $GitHubRepository,
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo',
    [switch] $Reconfigure
)

$ErrorActionPreference = 'Stop'
$VersionArguments = @{ Version = $Version }
if ($GitHubRepository) {
    $VersionArguments.GitHubRepository = $GitHubRepository
}
& (Join-Path $PSScriptRoot 'Set-OPBS-Version.ps1') @VersionArguments
& (Join-Path $PSScriptRoot 'Build-Presenter.ps1') -Configuration $Configuration -Reconfigure:$Reconfigure
& (Join-Path $PSScriptRoot 'Package-Presenter.ps1') -Configuration $Configuration
& (Join-Path $PSScriptRoot 'Build-OPBS-Installer.ps1') -Configuration $Configuration

Write-Host "La versión OPBS $Version quedó compilada. Revisa y confirma opbs-release.json antes de publicar."
