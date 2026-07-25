[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo',
    [string] $Generator = 'Visual Studio 18 2026',
    [ValidateRange(1, 64)]
    [int] $Jobs = 1,
    [switch] $Reconfigure
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDirectory = Join-Path $ProjectRoot 'build_x64'

$PortableCMake = Get-ChildItem -Path (Join-Path $ProjectRoot '.tools') -Filter cmake.exe -Recurse -ErrorAction SilentlyContinue |
    Select-Object -First 1 -ExpandProperty FullName
$CMake = if ($PortableCMake) { $PortableCMake } else { (Get-Command cmake -ErrorAction Stop).Source }

$ObsDependencies = Get-ChildItem -Path (Join-Path $ProjectRoot '.deps') -Directory -Filter 'obs-deps-*-x64' -ErrorAction SilentlyContinue |
    Where-Object Name -NotLike '*qt6*' | Sort-Object Name -Descending | Select-Object -First 1
$QtDependencies = Get-ChildItem -Path (Join-Path $ProjectRoot '.deps') -Directory -Filter 'obs-deps-qt6-*-x64' -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending | Select-Object -First 1

if (-not $ObsDependencies -or -not $QtDependencies) {
    throw 'Faltan .deps/obs-deps-*-x64 y/o .deps/obs-deps-qt6-*-x64. Consulta docs/PRESENTADOR_CONTINUIDAD.md.'
}

$Cache = Join-Path $BuildDirectory 'CMakeCache.txt'
if ($Reconfigure -or -not (Test-Path -LiteralPath $Cache)) {
    $PrefixPath = "$($ObsDependencies.FullName);$($QtDependencies.FullName)"
    & $CMake -S $ProjectRoot -B $BuildDirectory -G $Generator -A x64 "-DCMAKE_PREFIX_PATH=$PrefixPath" -DENABLE_BROWSER=OFF
    if ($LASTEXITCODE -ne 0) { throw "CMake no pudo configurar el proyecto (código $LASTEXITCODE)." }
}

& $CMake --build $BuildDirectory --config $Configuration --target obs-studio --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw "La compilación falló (código $LASTEXITCODE)." }

$BinaryDirectory = Join-Path $BuildDirectory "rundir/$Configuration/bin/64bit"
Set-Content -LiteralPath (Join-Path $BinaryDirectory 'disable_updater.txt') `
    -Value 'Las actualizaciones oficiales de OBS se integran mediante la revision protegida de Presentador.' `
    -Encoding ASCII

Write-Host "Compilación terminada: build_x64/rundir/$Configuration"
