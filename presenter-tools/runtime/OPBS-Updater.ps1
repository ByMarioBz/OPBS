[CmdletBinding()]
param(
    [int] $CurrentProcessId = 0,
    [switch] $CheckOnly,
    [switch] $Silent
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms

function Show-Information {
    param([string] $Text)
    if ($Silent) {
        return
    }
    if ($CheckOnly) {
        Write-Output $Text
        return
    }
    [void][Windows.Forms.MessageBox]::Show(
        $Text,
        'Actualizaciones de OPBS',
        [Windows.Forms.MessageBoxButtons]::OK,
        [Windows.Forms.MessageBoxIcon]::Information
    )
}

function Show-ErrorMessage {
    param([string] $Text)
    if ($Silent) {
        return
    }
    if ($CheckOnly) {
        Write-Output "ERROR: $Text"
        return
    }
    [void][Windows.Forms.MessageBox]::Show(
        $Text,
        'Actualizaciones de OPBS',
        [Windows.Forms.MessageBoxButtons]::OK,
        [Windows.Forms.MessageBoxIcon]::Error
    )
}

function ConvertTo-OPBSVersion {
    param([Parameter(Mandatory)][string] $Value)

    $Normalized = $Value.Trim()
    $Normalized = $Normalized -replace '^opbs-v', ''
    $Normalized = $Normalized -replace '^v', ''
    if ($Normalized -notmatch '^\d+\.\d+\.\d+$') {
        throw "La versión '$Value' no usa el formato mayor.menor.parche."
    }
    return [version]$Normalized
}

try {
    $ConfigurationPath = Join-Path $PSScriptRoot 'opbs-release.json'
    if (-not (Test-Path -LiteralPath $ConfigurationPath)) {
        throw 'No se encontró opbs-release.json junto al actualizador.'
    }

    $Configuration = Get-Content -Raw -LiteralPath $ConfigurationPath | ConvertFrom-Json
    $Repository = [string]$Configuration.githubRepository
    if ($Repository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
        throw 'El repositorio de actualizaciones de OPBS todavía no está configurado.'
    }

    $CurrentVersion = ConvertTo-OPBSVersion ([string]$Configuration.version)
    $Headers = @{
        Accept = 'application/vnd.github+json'
        'User-Agent' = "OPBS-Updater/$CurrentVersion"
        'X-GitHub-Api-Version' = '2026-03-10'
    }
    $Release = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repository/releases/latest" -Headers $Headers
    $AvailableVersion = ConvertTo-OPBSVersion ([string]$Release.tag_name)

    if ($AvailableVersion -le $CurrentVersion) {
        Show-Information "OPBS $CurrentVersion ya es la versión más reciente."
        exit 0
    }

    $InstallerAsset = $Release.assets | Where-Object name -eq $Configuration.installerAsset | Select-Object -First 1
    $ChecksumAsset = $Release.assets | Where-Object name -eq $Configuration.checksumAsset | Select-Object -First 1
    if (-not $InstallerAsset -or -not $ChecksumAsset) {
        throw "La versión $AvailableVersion no contiene el instalador y su archivo SHA-256."
    }

    if ($CheckOnly) {
        [PSCustomObject]@{
            currentVersion = $CurrentVersion.ToString(3)
            availableVersion = $AvailableVersion.ToString(3)
            installerUrl = $InstallerAsset.browser_download_url
            releaseUrl = $Release.html_url
        } | ConvertTo-Json
        exit 0
    }

    $Answer = [Windows.Forms.MessageBox]::Show(
        "Está disponible OPBS $AvailableVersion.`n`n¿Quieres descargar e instalar la actualización? OPBS se cerrará después de verificar el instalador.",
        'Actualización disponible',
        [Windows.Forms.MessageBoxButtons]::YesNo,
        [Windows.Forms.MessageBoxIcon]::Question
    )
    if ($Answer -ne [Windows.Forms.DialogResult]::Yes) {
        exit 0
    }

    $DownloadDirectory = Join-Path ([IO.Path]::GetTempPath()) "OPBS-Update-$AvailableVersion"
    New-Item -ItemType Directory -Path $DownloadDirectory -Force | Out-Null
    $InstallerPath = Join-Path $DownloadDirectory ([string]$Configuration.installerAsset)
    $ChecksumPath = Join-Path $DownloadDirectory ([string]$Configuration.checksumAsset)

    Invoke-WebRequest -Uri $InstallerAsset.browser_download_url -Headers $Headers -OutFile $InstallerPath
    Invoke-WebRequest -Uri $ChecksumAsset.browser_download_url -Headers $Headers -OutFile $ChecksumPath

    $ChecksumText = Get-Content -Raw -LiteralPath $ChecksumPath
    $ExpectedHash = [regex]::Match($ChecksumText, '(?i)\b[0-9a-f]{64}\b').Value.ToUpperInvariant()
    $ActualHash = (Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash.ToUpperInvariant()
    if (-not $ExpectedHash -or $ActualHash -ne $ExpectedHash) {
        Remove-Item -LiteralPath $InstallerPath -Force -ErrorAction SilentlyContinue
        throw 'El instalador descargado no coincide con el SHA-256 publicado. La actualización fue cancelada.'
    }

    if ($CurrentProcessId -gt 0) {
        Stop-Process -Id $CurrentProcessId -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $CurrentProcessId -Timeout 15 -ErrorAction SilentlyContinue
    }

    Start-Process -FilePath $InstallerPath
} catch {
    Show-ErrorMessage $_.Exception.Message
    exit 1
}
