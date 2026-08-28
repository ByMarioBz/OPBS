[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$RunDirectory = Join-Path $ProjectRoot "build_x64/rundir/$Configuration"
$Destination = Join-Path $ProjectRoot 'dist/OPBS'
$SourceExecutable = Join-Path $RunDirectory 'bin/64bit/obs64.exe'
$ReleaseConfigurationPath = Join-Path $PSScriptRoot 'opbs-release.json'
$ReleaseConfiguration = Get-Content -Raw -LiteralPath $ReleaseConfigurationPath | ConvertFrom-Json

if (-not (Test-Path -LiteralPath $SourceExecutable)) {
    throw "No existe $SourceExecutable. Ejecuta primero Build-Presenter.ps1."
}

New-Item -ItemType Directory -Path $Destination -Force | Out-Null

$LegacyConfiguration = Join-Path $ProjectRoot 'dist/PresentadorMultimedia/config'
$DestinationConfiguration = Join-Path $Destination 'config'
if (-not (Test-Path -LiteralPath $DestinationConfiguration) -and (Test-Path -LiteralPath $LegacyConfiguration)) {
    Copy-Item -LiteralPath $LegacyConfiguration -Destination $DestinationConfiguration -Recurse
}

foreach ($DirectoryName in @('bin', 'data', 'obs-plugins')) {
    $GeneratedDestination = Join-Path $Destination $DirectoryName
    if (Test-Path -LiteralPath $GeneratedDestination) {
        Remove-Item -LiteralPath $GeneratedDestination -Recurse -Force
    }
    Copy-Item -LiteralPath (Join-Path $RunDirectory $DirectoryName) -Destination $GeneratedDestination -Recurse
}

$BinaryDirectory = Join-Path $Destination 'bin/64bit'
$CopiedObsExecutable = Join-Path $BinaryDirectory 'obs64.exe'
$InstalledExecutableName = 'Presenter Broadcast Studio.exe'
$InstalledExecutable = Join-Path $BinaryDirectory $InstalledExecutableName
if (Test-Path -LiteralPath $InstalledExecutable) {
    Remove-Item -LiteralPath $InstalledExecutable -Force
}
Rename-Item -LiteralPath $CopiedObsExecutable -NewName $InstalledExecutableName
Set-Content -LiteralPath (Join-Path $BinaryDirectory 'disable_updater.txt') `
    -Value 'OPBS utiliza exclusivamente su actualizador de GitHub Releases.' -Encoding ASCII
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'runtime/OPBS-Updater.ps1') -Destination $BinaryDirectory -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'runtime/OPBS-Launcher.ps1') -Destination $BinaryDirectory -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'runtime/OPBS-MigrateData.ps1') -Destination $BinaryDirectory -Force
Copy-Item -LiteralPath $ReleaseConfigurationPath -Destination (Join-Path $BinaryDirectory 'opbs-release.json') -Force

$Launcher = @'
@echo off
cd /d "%~dp0bin\64bit"
start "Presenter Broadcast Studio" "Presenter Broadcast Studio.exe" --portable --disable-updater
'@
Set-Content -LiteralPath (Join-Path $Destination 'INICIAR_OPBS.bat') -Value $Launcher -Encoding ASCII

Write-Host "OPBS $($ReleaseConfiguration.version) empaquetado en: $Destination"
Write-Warning 'La carpeta config conserva preferencias y rutas locales existentes; no contiene los archivos multimedia.'
