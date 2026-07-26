[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$Candidates = @(
    (Join-Path ${env:ProgramFiles(x86)} 'NSIS\makensis.exe'),
    (Join-Path $env:ProgramFiles 'NSIS\makensis.exe')
)
$Existing = $Candidates | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
if ($Existing) {
    Write-Host "NSIS disponible en: $Existing"
    exit 0
}

$Winget = Get-Command winget.exe -ErrorAction Stop
& $Winget.Source install --id NSIS.NSIS --exact --silent --accept-package-agreements --accept-source-agreements `
    --disable-interactivity
if ($LASTEXITCODE -ne 0) {
    throw "winget no pudo instalar NSIS (código $LASTEXITCODE)."
}

$Installed = $Candidates | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
if (-not $Installed) {
    throw 'NSIS terminó de instalarse, pero makensis.exe no apareció en la ruta esperada.'
}
Write-Host "NSIS instalado en: $Installed"
