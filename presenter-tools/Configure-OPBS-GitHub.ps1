[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [string] $Repository
)

$ErrorActionPreference = 'Stop'
if ($Repository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
    throw 'Usa el formato propietario/repositorio.'
}

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$OriginUrl = "https://github.com/$Repository.git"
$ExistingOrigin = @(& git -C $ProjectRoot remote get-url origin 2>$null)
if ($ExistingOrigin.Count -eq 0) {
    & git -C $ProjectRoot remote add origin $OriginUrl
    if ($LASTEXITCODE -ne 0) {
        throw 'No se pudo crear el remoto origin.'
    }
} elseif ($ExistingOrigin[0].Trim() -ne $OriginUrl) {
    throw "origin ya apunta a $($ExistingOrigin[0]). No se reemplazó automáticamente."
}

$ConfigurationPath = Join-Path $PSScriptRoot 'opbs-release.json'
$Configuration = Get-Content -Raw -LiteralPath $ConfigurationPath | ConvertFrom-Json
& (Join-Path $PSScriptRoot 'Set-OPBS-Version.ps1') -Version $Configuration.version -GitHubRepository $Repository

Write-Host "OPBS quedó asociado a $OriginUrl"
Write-Host 'El repositorio debe ser público para que las aplicaciones instaladas consulten Releases sin credenciales.'
