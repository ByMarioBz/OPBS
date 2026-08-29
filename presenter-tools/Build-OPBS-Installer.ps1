[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ConfigurationData = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'opbs-release.json') | ConvertFrom-Json
$Version = [string]$ConfigurationData.version
if ($Version -notmatch '^\d+\.\d+\.\d+(?:[A-Za-z][A-Za-z0-9.-]*)?$') {
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

& (Join-Path $PSScriptRoot 'Create-Portable.ps1') -Configuration $Configuration -OutputDirectory '.installer-payload'

$PayloadDirectory = Join-Path $ProjectRoot '.installer-payload'
foreach ($PortableOnlyFile in @('portable_mode.txt', 'INICIAR_OPBS.bat', 'LEEME.txt')) {
    $PortableOnlyPath = Join-Path $PayloadDirectory $PortableOnlyFile
    if (Test-Path -LiteralPath $PortableOnlyPath) {
        Remove-Item -LiteralPath $PortableOnlyPath -Force
    }
}
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
$StaleScriptingFiles = @(
    Get-ChildItem -LiteralPath $PayloadDirectory -Recurse -File |
        Where-Object { $_.FullName -match 'obs-scripting|obslua|obspython' }
)
if ($StaleScriptingFiles.Count -gt 0) {
    throw 'La entrega portable contiene módulos de scripting desactivados. Se canceló la publicación.'
}

$ReleaseDirectory = Join-Path $ProjectRoot "release/$Version"
New-Item -ItemType Directory -Path $ReleaseDirectory -Force | Out-Null
$InstallerPath = Join-Path $ReleaseDirectory ([string]$ConfigurationData.installerAsset)
$ChecksumPath = Join-Path $ReleaseDirectory ([string]$ConfigurationData.checksumAsset)

foreach ($Output in @($InstallerPath, $ChecksumPath)) {
    if (Test-Path -LiteralPath $Output) {
        Remove-Item -LiteralPath $Output -Force
    }
}

$NumericVersion = if ($Version -match '^(\d+\.\d+\.\d+)') { $Matches[1] } else { throw 'La versión no tiene un núcleo numérico válido.' }
$FileVersion = "$NumericVersion.0"
$InstallerScript = Join-Path $ProjectRoot 'installer/OPBS.nsi'
& $MakeNsis "/DOPBS_VERSION=$Version" "/DOPBS_FILE_VERSION=$FileVersion" `
    "/DPAYLOAD_DIR=$PayloadDirectory" "/DOUTPUT_FILE=$InstallerPath" $InstallerScript
if ($LASTEXITCODE -ne 0) {
    throw "NSIS no pudo crear el instalador (código $LASTEXITCODE)."
}

$Sha256 = [System.Security.Cryptography.SHA256]::Create()
$InstallerStream = [System.IO.File]::OpenRead($InstallerPath)
try {
    $InstallerHash = ([System.BitConverter]::ToString($Sha256.ComputeHash($InstallerStream))).Replace('-', '').ToLowerInvariant()
} finally {
    $InstallerStream.Dispose()
    $Sha256.Dispose()
}
Set-Content -LiteralPath $ChecksumPath -Value "$InstallerHash  $($ConfigurationData.installerAsset)" -Encoding ASCII

Write-Host "Instalador: $InstallerPath"
Write-Host "SHA-256: $ChecksumPath"
