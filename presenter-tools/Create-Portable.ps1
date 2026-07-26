[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo',
    [string] $OutputDirectory = 'portable'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$RunDirectory = Join-Path $ProjectRoot "build_x64/rundir/$Configuration"
$SourceExecutable = Join-Path $RunDirectory 'bin/64bit/obs64.exe'
$ReleaseConfigurationPath = Join-Path $PSScriptRoot 'opbs-release.json'
$ReleaseConfiguration = Get-Content -Raw -LiteralPath $ReleaseConfigurationPath | ConvertFrom-Json
$Destination = if ([IO.Path]::IsPathRooted($OutputDirectory)) {
    [IO.Path]::GetFullPath($OutputDirectory)
} else {
    [IO.Path]::GetFullPath((Join-Path $ProjectRoot $OutputDirectory))
}
$ProjectBoundary = $ProjectRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar

if (-not $Destination.StartsWith($ProjectBoundary, [StringComparison]::OrdinalIgnoreCase) -or
    $Destination -eq $ProjectRoot) {
    throw 'La carpeta portable debe estar dentro del repositorio y no puede ser su raíz.'
}
if (-not (Test-Path -LiteralPath $SourceExecutable)) {
    throw "No existe $SourceExecutable. Ejecuta primero Build-Presenter.ps1."
}

if (Test-Path -LiteralPath $Destination) {
    Remove-Item -LiteralPath $Destination -Recurse -Force
}
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
foreach ($DirectoryName in @('bin', 'data', 'obs-plugins')) {
    Copy-Item -LiteralPath (Join-Path $RunDirectory $DirectoryName) `
        -Destination (Join-Path $Destination $DirectoryName) -Recurse
}

# Los símbolos de depuración contienen rutas absolutas del equipo de compilación y no
# son necesarios para ejecutar OPBS. Nunca deben formar parte de una entrega pública.
Get-ChildItem -LiteralPath $Destination -Recurse -File |
    Where-Object { $_.Extension -in @('.pdb', '.ilk') } |
    Remove-Item -Force

# Una compilación incremental puede conservar módulos de funciones ahora
# desactivadas. El portable de OPBS no debe arrastrar scripting obsoleto.
$StaleScriptingTargets = @(
    (Join-Path $Destination 'bin/64bit/obs-scripting.dll'),
    (Join-Path $Destination 'data/obs-scripting')
)
foreach ($StaleTarget in $StaleScriptingTargets) {
    if (Test-Path -LiteralPath $StaleTarget) {
        Remove-Item -LiteralPath $StaleTarget -Recurse -Force
    }
}

$PortableBin = Join-Path $Destination 'bin/64bit'
$CopiedObsExecutable = Join-Path $PortableBin 'obs64.exe'
Rename-Item -LiteralPath $CopiedObsExecutable -NewName 'OPBS.exe'
Set-Content -LiteralPath (Join-Path $PortableBin 'disable_updater.txt') `
    -Value 'OPBS utiliza exclusivamente su actualizador de GitHub Releases.' -Encoding ASCII
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'runtime/OPBS-Updater.ps1') -Destination $PortableBin -Force
Copy-Item -LiteralPath $ReleaseConfigurationPath -Destination (Join-Path $PortableBin 'opbs-release.json') -Force

$Launcher = @'
@echo off
cd /d "%~dp0bin\64bit"
start "OPBS" OPBS.exe --portable --disable-updater
'@
Set-Content -LiteralPath (Join-Path $Destination 'INICIAR_OPBS.bat') -Value $Launcher -Encoding ASCII
Set-Content -LiteralPath (Join-Path $Destination 'portable_mode.txt') -Value 'OPBS portable' -Encoding ASCII

$Readme = @"
OPBS $($ReleaseConfiguration.version)

1. Abre INICIAR_OPBS.bat.
2. También puedes abrir bin\64bit\OPBS.exe directamente.
3. La configuración se guarda dentro de esta carpeta.
4. Los archivos multimedia importados no se copian; deben seguir disponibles en sus rutas originales.
5. Las Biblias y preferencias locales no se incluyen en las entregas públicas.
"@
Set-Content -LiteralPath (Join-Path $Destination 'LEEME.txt') -Value $Readme -Encoding UTF8

Write-Host "OPBS portable creado en: $Destination"
Write-Host "Ejecutable: $(Join-Path $PortableBin 'OPBS.exe')"
