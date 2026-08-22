[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Configuration = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'opbs-release.json') | ConvertFrom-Json
$Version = [string]$Configuration.version
$Repository = [string]$Configuration.githubRepository
if ($Repository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
    throw 'Configura githubRepository como propietario/repositorio antes de publicar.'
}
if (& git -C $ProjectRoot status --porcelain) {
    throw 'El árbol de Git debe estar limpio antes de publicar una versión.'
}

$Gh = Get-Command gh.exe -ErrorAction Stop
& $Gh.Source auth status
if ($LASTEXITCODE -ne 0) {
    throw 'GitHub CLI no tiene una sesión válida. Ejecuta gh auth login.'
}
$Visibility = (& $Gh.Source repo view $Repository --json visibility --jq '.visibility').Trim()
if ($LASTEXITCODE -ne 0 -or $Visibility -ne 'PUBLIC') {
    throw 'El actualizador sin credenciales requiere que el repositorio y sus Releases sean públicos.'
}

$Tag = "opbs-v$Version"
$Head = (& git -C $ProjectRoot rev-parse HEAD).Trim()
$TagCommitOutput = @(& git -C $ProjectRoot rev-list -n 1 $Tag 2>$null)
$TagCommit = if ($TagCommitOutput.Count -gt 0) { $TagCommitOutput[0].Trim() } else { '' }
if (-not $TagCommit -or $TagCommit -ne $Head) {
    throw "Crea el tag anotado $Tag en el commit actual y súbelo antes de publicar."
}

$ReleaseDirectory = Join-Path $ProjectRoot "release/$Version"
$Assets = @(
    (Join-Path $ReleaseDirectory ([string]$Configuration.installerAsset)),
    (Join-Path $ReleaseDirectory ([string]$Configuration.checksumAsset))
)
foreach ($Asset in $Assets) {
    if (-not (Test-Path -LiteralPath $Asset)) {
        throw "Falta el artefacto $Asset. Ejecuta Build-OPBS-Release.cmd."
    }
}

$ReleaseNotesPath = Join-Path $ProjectRoot "docs/releases/OPBS-$Version.md"
if (-not (Test-Path -LiteralPath $ReleaseNotesPath)) {
    throw "Faltan las notas de la versión en $ReleaseNotesPath."
}
$ReleaseNotes = [IO.File]::ReadAllText($ReleaseNotesPath, [Text.Encoding]::UTF8)
$Sha256 = [System.Security.Cryptography.SHA256]::Create()
$InstallerStream = [System.IO.File]::OpenRead($Assets[0])
try {
    $InstallerHash = ([System.BitConverter]::ToString($Sha256.ComputeHash($InstallerStream))).Replace('-', '').ToLowerInvariant()
} finally {
    $InstallerStream.Dispose()
    $Sha256.Dispose()
}
$PublishedNotesPath = Join-Path $ReleaseDirectory 'release-notes.md'
$PublishedNotes = @"
$($ReleaseNotes.Trim())

## Checksums SHA-256

    $([IO.Path]::GetFileName($Assets[0])): $InstallerHash
"@
$Utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)
[IO.File]::WriteAllText($PublishedNotesPath, "$PublishedNotes`n", $Utf8WithoutBom)

& $Gh.Source release create $Tag @Assets --repo $Repository --verify-tag --title "OPBS $Version" `
    --notes-file $PublishedNotesPath --latest
if ($LASTEXITCODE -ne 0) {
    throw "GitHub CLI no pudo publicar OPBS $Version."
}
Write-Host "OPBS $Version publicado en GitHub Releases."
