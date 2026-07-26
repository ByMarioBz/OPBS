[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ConfigurationData = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'opbs-release.json') | ConvertFrom-Json
$Version = [string]$ConfigurationData.version
if ($Version -notmatch '^\d+\.\d+\.\d+$') {
    throw 'La versión configurada de OPBS no es válida.'
}

$MakeNsisCandidates = @(
    (Join-Path ${env:ProgramFiles(x86)} 'NSIS\makensis.exe'),
    (Join-Path $env:ProgramFiles 'NSIS\makensis.exe')
)
$MakeNsis = $MakeNsisCandidates | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
if (-not $MakeNsis) {
    throw 'Falta NSIS. Ejecuta presenter-tools\Ensure-OPBS-InstallerTools.cmd.'
}

& (Join-Path $PSScriptRoot 'Create-Portable.ps1') -Configuration $Configuration

$PayloadDirectory = Join-Path $ProjectRoot 'portable'
$PrivateConfiguration = Join-Path $PayloadDirectory 'config'
if (Test-Path -LiteralPath $PrivateConfiguration) {
    $PrivateFiles = @(Get-ChildItem -LiteralPath $PrivateConfiguration -Recurse -File)
    if ($PrivateFiles.Count -gt 0) {
        throw 'La entrega portable contiene configuración o datos locales. Se canceló la publicación.'
    }
}
$DebugFiles = @(
    Get-ChildItem -LiteralPath $PayloadDirectory -Recurse -File |
        Where-Object { $_.Extension -in @('.pdb', '.ilk') }
)
if ($DebugFiles.Count -gt 0) {
    throw 'La entrega portable contiene símbolos de depuración con rutas locales. Se canceló la publicación.'
}

$ReleaseDirectory = Join-Path $ProjectRoot "release/$Version"
New-Item -ItemType Directory -Path $ReleaseDirectory -Force | Out-Null
$InstallerPath = Join-Path $ReleaseDirectory ([string]$ConfigurationData.installerAsset)
$PortablePath = Join-Path $ReleaseDirectory ([string]$ConfigurationData.portableAsset)
$ChecksumPath = Join-Path $ReleaseDirectory ([string]$ConfigurationData.checksumAsset)

foreach ($Output in @($InstallerPath, $PortablePath, $ChecksumPath)) {
    if (Test-Path -LiteralPath $Output) {
        Remove-Item -LiteralPath $Output -Force
    }
}

$VersionParts = $Version.Split('.')
$FileVersion = "$($VersionParts[0]).$($VersionParts[1]).$($VersionParts[2]).0"
$InstallerScript = Join-Path $ProjectRoot 'installer/OPBS.nsi'
& $MakeNsis "/DOPBS_VERSION=$Version" "/DOPBS_FILE_VERSION=$FileVersion" `
    "/DPAYLOAD_DIR=$PayloadDirectory" "/DOUTPUT_FILE=$InstallerPath" $InstallerScript
if ($LASTEXITCODE -ne 0) {
    throw "NSIS no pudo crear el instalador (código $LASTEXITCODE)."
}

Compress-Archive -Path (Join-Path $PayloadDirectory '*') -DestinationPath $PortablePath -CompressionLevel Optimal
$InstallerHash = (Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath $ChecksumPath -Value "$InstallerHash  $($ConfigurationData.installerAsset)" -Encoding ASCII

Write-Host "Instalador: $InstallerPath"
Write-Host "Portable: $PortablePath"
Write-Host "SHA-256: $ChecksumPath"
