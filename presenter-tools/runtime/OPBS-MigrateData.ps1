[CmdletBinding()]
param(
    [string] $InstallRoot = (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)),
    [string] $DestinationRoot = (Join-Path $env:APPDATA 'opbs')
)

$ErrorActionPreference = 'Stop'

function Copy-MissingTree {
    param(
        [Parameter(Mandatory)][string] $Source,
        [Parameter(Mandatory)][string] $Destination
    )

    if (-not (Test-Path -LiteralPath $Source)) {
        return
    }

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    foreach ($Item in Get-ChildItem -LiteralPath $Source -Recurse -Force) {
        $RelativePath = $Item.FullName.Substring($Source.TrimEnd('\').Length).TrimStart('\')
        if ([string]::IsNullOrWhiteSpace($RelativePath) -or $RelativePath -like '.sentinel*') {
            continue
        }
        $Target = Join-Path $Destination $RelativePath
        if ($Item.PSIsContainer) {
            New-Item -ItemType Directory -Path $Target -Force | Out-Null
        } elseif (-not (Test-Path -LiteralPath $Target)) {
            New-Item -ItemType Directory -Path (Split-Path -Parent $Target) -Force | Out-Null
            Copy-Item -LiteralPath $Item.FullName -Destination $Target
        }
    }
}

$Sources = @(
    (Join-Path $InstallRoot 'config\opbs'),
    (Join-Path $InstallRoot 'config\obs-studio')
)
foreach ($Source in $Sources) {
    Copy-MissingTree -Source $Source -Destination $DestinationRoot
}

New-Item -ItemType Directory -Path $DestinationRoot -Force | Out-Null
$Marker = [ordered]@{
    migratedAt = [DateTime]::UtcNow.ToString('o')
    installRoot = $InstallRoot
    sources = @($Sources | Where-Object { Test-Path -LiteralPath $_ })
}
$Utf8WithoutBom = New-Object Text.UTF8Encoding($false)
[IO.File]::WriteAllText(
    (Join-Path $DestinationRoot 'opbs-data-migration.json'),
    (($Marker | ConvertTo-Json -Depth 3) + "`n"),
    $Utf8WithoutBom
)
