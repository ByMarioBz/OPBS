[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [string] $Version,
    [string] $GitHubRepository
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$CurrentBranch = (& git -C $ProjectRoot branch --show-current).Trim()
if ($LASTEXITCODE -ne 0 -or $CurrentBranch -ne 'feature/media-presenter') {
    throw 'La versión de OPBS solo se cambia desde feature/media-presenter.'
}
if ($Version -notmatch '^\d+\.\d+\.\d+(?:[A-Za-z][A-Za-z0-9.-]*)?$') {
	throw 'La versión debe usar mayor.menor.parche y puede incluir un sufijo beta, por ejemplo 0.1.7b.'
}
if ($GitHubRepository -and $GitHubRepository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
    throw 'GitHubRepository debe usar el formato propietario/repositorio.'
}

$ConfigurationPath = Join-Path $PSScriptRoot 'opbs-release.json'
$Configuration = Get-Content -Raw -LiteralPath $ConfigurationPath | ConvertFrom-Json
$Configuration.version = $Version
if ($GitHubRepository) {
    $Configuration.githubRepository = $GitHubRepository
}
$Json = $Configuration | ConvertTo-Json -Depth 4
$Utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($ConfigurationPath, "$Json`n", $Utf8WithoutBom)

Write-Host "OPBS configurado como versión $Version."
if (-not $Configuration.githubRepository) {
    Write-Warning 'Falta configurar propietario/repositorio para activar actualizaciones y publicación.'
}
