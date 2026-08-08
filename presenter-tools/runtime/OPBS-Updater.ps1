[CmdletBinding()]
param(
    [int] $CurrentProcessId = 0,
    [switch] $CheckOnly,
    [switch] $Silent
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

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

function Show-UpdatePrompt {
    param(
        [Parameter(Mandatory)][version] $AvailableVersion,
        [string] $ReleaseNotes
    )

    $Form = New-Object Windows.Forms.Form
    $Form.Text = "Actualización disponible - OPBS $AvailableVersion"
    $Form.StartPosition = [Windows.Forms.FormStartPosition]::CenterScreen
    $Form.Size = New-Object Drawing.Size(760, 650)
    $Form.MinimumSize = New-Object Drawing.Size(620, 480)
    $Form.ShowIcon = $false

    $Header = New-Object Windows.Forms.Label
    $Header.Dock = [Windows.Forms.DockStyle]::Top
    $Header.Height = 76
    $Header.Padding = New-Object Windows.Forms.Padding(16, 14, 16, 8)
    $Header.Font = New-Object Drawing.Font($Header.Font.FontFamily, 11, [Drawing.FontStyle]::Bold)
    $Header.Text = "Está disponible OPBS $AvailableVersion.`r`nRevisa las novedades antes de instalar."
    $Form.Controls.Add($Header)

    $Notes = New-Object Windows.Forms.RichTextBox
    $Notes.Dock = [Windows.Forms.DockStyle]::Fill
    $Notes.ReadOnly = $true
    $Notes.DetectUrls = $true
    $Notes.BackColor = [Drawing.SystemColors]::Window
    $Notes.BorderStyle = [Windows.Forms.BorderStyle]::FixedSingle
    $Notes.Font = New-Object Drawing.Font('Segoe UI', 9.5)
    $Notes.Text = if ([string]::IsNullOrWhiteSpace($ReleaseNotes)) {
        "Consulta las notas completas en el Release de GitHub."
    } else {
        $ReleaseNotes.Trim()
    }
    $Form.Controls.Add($Notes)
    $Notes.BringToFront()

    $Buttons = New-Object Windows.Forms.FlowLayoutPanel
    $Buttons.Dock = [Windows.Forms.DockStyle]::Bottom
    $Buttons.Height = 62
    $Buttons.Padding = New-Object Windows.Forms.Padding(8, 12, 12, 8)
    $Buttons.FlowDirection = [Windows.Forms.FlowDirection]::RightToLeft

    $Install = New-Object Windows.Forms.Button
    $Install.Text = 'Descargar e instalar'
    $Install.AutoSize = $true
    $Install.DialogResult = [Windows.Forms.DialogResult]::Yes
    $Buttons.Controls.Add($Install)

    $Cancel = New-Object Windows.Forms.Button
    $Cancel.Text = 'Ahora no'
    $Cancel.AutoSize = $true
    $Cancel.DialogResult = [Windows.Forms.DialogResult]::No
    $Buttons.Controls.Add($Cancel)

    $Form.AcceptButton = $Install
    $Form.CancelButton = $Cancel
    $Form.Controls.Add($Buttons)
    $Buttons.BringToFront()
    $Form.Add_Shown({ $Form.Activate() })

    return $Form.ShowDialog() -eq [Windows.Forms.DialogResult]::Yes
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
            releaseNotes = [string]$Release.body
        } | ConvertTo-Json
        exit 0
    }

    if (-not (Show-UpdatePrompt -AvailableVersion $AvailableVersion -ReleaseNotes ([string]$Release.body))) {
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
