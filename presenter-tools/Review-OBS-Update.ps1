[CmdletBinding()]
param(
    [string] $TargetVersion,
    [switch] $SkipFetch,
    [string] $ReportPath
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$PolicyPath = Join-Path $PSScriptRoot 'obs-upstream-policy.json'
$Policy = Get-Content -Raw -LiteralPath $PolicyPath | ConvertFrom-Json

function Invoke-Git {
    param([Parameter(Mandatory)][string[]] $Arguments)

    $Output = & git -C $ProjectRoot @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Git falló: git $($Arguments -join ' ')`n$($Output -join "`n")"
    }
    return @($Output)
}

if ((Invoke-Git @('branch', '--show-current')) -ne $Policy.productBranch) {
    throw "Ejecuta la revisión desde la rama $($Policy.productBranch)."
}

$PushUrl = (Invoke-Git @('remote', 'get-url', '--push', 'obs-public') | Select-Object -First 1).Trim()
if ($PushUrl -ne 'DISABLED') {
    throw 'El remoto obs-public debe conservar el envío deshabilitado.'
}

if (-not $SkipFetch) {
    Write-Host 'Consultando etiquetas oficiales de OBS...'
    Invoke-Git @('fetch', '--prune', '--tags', 'obs-public') | Out-Null
}

if (-not $TargetVersion) {
    $StableTags = foreach ($Tag in Invoke-Git @('tag', '--merged', 'obs-public/master', '--list')) {
        $Name = $Tag.Trim()
        if ($Name -match '^\d+\.\d+\.\d+$') {
            [PSCustomObject]@{
                Name = $Name
                Version = [version]$Name
            }
        }
    }
    $TargetVersion = ($StableTags | Sort-Object Version -Descending | Select-Object -First 1).Name
}

if ($TargetVersion -notmatch '^\d+\.\d+\.\d+$') {
    throw "La versión objetivo debe ser una etiqueta estable, por ejemplo 32.2.1."
}

$TargetCommit = (Invoke-Git @('rev-parse', "$TargetVersion^{commit}") | Select-Object -First 1).Trim()
$IntegratedCommit = $Policy.lastIntegratedUpstream.commit

& git -C $ProjectRoot merge-base --is-ancestor $IntegratedCommit $TargetCommit
if ($LASTEXITCODE -ne 0) {
    throw "$TargetVersion no continúa la base integrada $($Policy.lastIntegratedUpstream.version)."
}

$ChangedPaths = @(Invoke-Git @('diff', '--name-only', "$IntegratedCommit..$TargetCommit") |
    ForEach-Object { $_.Trim().Replace('\', '/') } | Where-Object { $_ })
$ProductPaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($Path in Invoke-Git @('diff', '--name-only', "$($Policy.historicalBase.commit)...HEAD")) {
    [void]$ProductPaths.Add($Path.Trim().Replace('\', '/'))
}

$Rows = foreach ($Path in $ChangedPaths) {
    $Classification = 'AISLADO'
    $Reason = 'No coincide con un archivo modificado por Presentador.'

    if ($ProductPaths.Contains($Path)) {
        $Classification = 'CONFLICTO'
        $Reason = 'El archivo también fue modificado por Presentador.'
    } else {
        foreach ($Prefix in $Policy.protectedPrefixes) {
            if ($Path.StartsWith($Prefix, [StringComparison]::OrdinalIgnoreCase)) {
                $Classification = 'SENSIBLE'
                $Reason = "Pertenece al área protegida $Prefix."
                break
            }
        }
    }

    [PSCustomObject]@{
        Classification = $Classification
        Path = $Path
        Reason = $Reason
    }
}

$Commits = @(Invoke-Git @('log', '--reverse', '--format=%h%x09%s', "$IntegratedCommit..$TargetCommit"))
$ConflictCount = @($Rows | Where-Object Classification -eq 'CONFLICTO').Count
$SensitiveCount = @($Rows | Where-Object Classification -eq 'SENSIBLE').Count
$IsolatedCount = @($Rows | Where-Object Classification -eq 'AISLADO').Count
$GeneratedAt = Get-Date -Format 'yyyy-MM-dd HH:mm:ss K'

$Report = [Collections.Generic.List[string]]::new()
$Report.Add("# Revisión de OBS $TargetVersion")
$Report.Add('')
$Report.Add("- Generada: $GeneratedAt")
$Report.Add("- Base integrada: $($Policy.lastIntegratedUpstream.version) ($IntegratedCommit)")
$Report.Add("- Objetivo oficial: $TargetVersion ($TargetCommit)")
$Report.Add("- Resultado: $ConflictCount conflicto(s), $SensitiveCount cambio(s) sensible(s), $IsolatedCount cambio(s) aislado(s)")
$Report.Add('')
$Report.Add('## Commits oficiales')
$Report.Add('')
foreach ($Commit in $Commits) {
    $Report.Add("- $Commit")
}
$Report.Add('')
$Report.Add('## Archivos cambiados')
$Report.Add('')
$Report.Add('| Clasificación | Archivo | Motivo |')
$Report.Add('|---|---|---|')
foreach ($Row in $Rows) {
    $Report.Add("| $($Row.Classification) | ``$($Row.Path)`` | $($Row.Reason) |")
}
$Report.Add('')
$Report.Add('## Regla de aplicación')
$Report.Add('')
$Report.Add('`AISLADO` significa que el cambio no pisa actualmente una personalización; no significa que sea necesario.')
$Report.Add('Antes de integrar un commit se debe justificar su utilidad, hacerlo en una rama de revisión y superar compilación y pruebas.')

if ($ReportPath) {
    $ResolvedReport = if ([IO.Path]::IsPathRooted($ReportPath)) {
        [IO.Path]::GetFullPath($ReportPath)
    } else {
        [IO.Path]::GetFullPath((Join-Path $ProjectRoot $ReportPath))
    }
    $ProjectRootBoundary = $ProjectRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (-not $ResolvedReport.StartsWith($ProjectRootBoundary, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'El reporte debe guardarse dentro del repositorio.'
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $ResolvedReport) -Force | Out-Null
    Set-Content -LiteralPath $ResolvedReport -Value $Report -Encoding UTF8
    Write-Host "Reporte guardado en: $ResolvedReport"
}

$Report
