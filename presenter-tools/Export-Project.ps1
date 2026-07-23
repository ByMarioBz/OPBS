[CmdletBinding()]
param(
    [string] $OutputDirectory,
    [switch] $IncludePortableApp
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutputDirectory) { $OutputDirectory = Split-Path -Parent $ProjectRoot }
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)

Push-Location $ProjectRoot
try {
    $Pending = git status --porcelain
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo consultar el repositorio Git.' }
    if ($Pending) { throw 'Hay cambios sin commit. Confírmalos antes de exportar para que el historial sea completo.' }

    New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
    $Stamp = Get-Date -Format 'yyyyMMdd-HHmm'
    $Bundle = Join-Path $OutputDirectory "PresentadorMultimedia-$Stamp.bundle"
    # Export product branches and tags, but omit editor/assistant implementation refs.
    git bundle create $Bundle --branches --tags
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo crear el Git bundle.' }
    git bundle verify $Bundle
    if ($LASTEXITCODE -ne 0) { throw 'La verificación del Git bundle falló.' }

    Write-Host "Historial portátil: $Bundle"

    if ($IncludePortableApp) {
        $PortableApp = Join-Path $ProjectRoot 'dist/PresentadorMultimedia'
        if (-not (Test-Path -LiteralPath (Join-Path $PortableApp 'bin/64bit/obs64.exe'))) {
            throw 'No existe la aplicación portátil. Ejecuta Package-Presenter.ps1.'
        }
        $Archive = Join-Path $OutputDirectory "PresentadorMultimedia-App-$Stamp.zip"
        Compress-Archive -Path $PortableApp -DestinationPath $Archive -CompressionLevel Optimal
        Write-Host "Aplicación portátil: $Archive"
        Write-Warning 'El ZIP puede contener preferencias y rutas locales, pero no incluye los medios referenciados.'
    }
}
finally {
    Pop-Location
}
