[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$Gh = Get-Command gh.exe -ErrorAction SilentlyContinue
if (-not $Gh) {
    $Winget = Get-Command winget.exe -ErrorAction Stop
    & $Winget.Source install --id GitHub.cli --exact --silent --accept-package-agreements `
        --accept-source-agreements --disable-interactivity
    if ($LASTEXITCODE -ne 0) {
        throw "winget no pudo instalar GitHub CLI (código $LASTEXITCODE)."
    }
    $Gh = Get-Command gh.exe -ErrorAction SilentlyContinue
}
if (-not $Gh) {
    throw 'GitHub CLI se instaló, pero esta terminal todavía no encuentra gh.exe. Abre una terminal nueva.'
}

Write-Host "GitHub CLI disponible en: $($Gh.Source)"
Write-Host 'Si aún no has iniciado sesión, ejecuta manualmente: gh auth login'
