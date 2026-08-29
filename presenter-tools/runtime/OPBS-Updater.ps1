[CmdletBinding()]
param(
    [int] $CurrentProcessId = 0,
    [switch] $CheckOnly,
    [switch] $Silent,
    [switch] $LaunchApp,
    [switch] $SafeMode,
    [string] $CurrentVersionOverride,
    [string] $LocalReleaseDirectory,
    [string] $InstallRootOverride,
    [string] $ReleaseApiUrlOverride,
    [ValidateRange(500, 10000)]
    [int] $MetadataTimeoutMilliseconds = 2000
)

$ErrorActionPreference = 'Stop'

function Initialize-OPBSUpdaterUi {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
}

function Get-OPBSLatestRelease {
    param(
        [Parameter(Mandatory)][string] $Uri,
        [Parameter(Mandatory)][hashtable] $Headers,
        [Parameter(Mandatory)][int] $TimeoutMilliseconds
    )

    Add-Type -AssemblyName System.Net.Http
    $Handler = New-Object System.Net.Http.HttpClientHandler
    $Client = New-Object System.Net.Http.HttpClient($Handler)
    $Client.Timeout = [TimeSpan]::FromMilliseconds($TimeoutMilliseconds)
    try {
        foreach ($Header in $Headers.GetEnumerator()) {
            [void]$Client.DefaultRequestHeaders.TryAddWithoutValidation([string]$Header.Key, [string]$Header.Value)
        }
        $Response = $Client.GetAsync($Uri).GetAwaiter().GetResult()
        try {
            $Response.EnsureSuccessStatusCode()
            $Json = $Response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
        } finally {
            $Response.Dispose()
        }
        return $Json | ConvertFrom-Json
    } finally {
        $Client.Dispose()
        $Handler.Dispose()
    }
}

function Start-OPBS {
    if (-not $LaunchApp) { return }
    $Executable = Join-Path $PSScriptRoot 'Presenter Broadcast Studio.exe'
    if (-not (Test-Path -LiteralPath $Executable)) {
        # Permite completar una transición desde las versiones cuyo ejecutable visible era OPBS.exe.
        $Executable = Join-Path $PSScriptRoot 'OPBS.exe'
    }
    $Arguments = @('--disable-updater')
    if ($SafeMode) { $Arguments += '--safe-mode' }
    Start-Process -FilePath $Executable -ArgumentList $Arguments -WorkingDirectory $PSScriptRoot
}

function Show-Information([string] $Text) {
    if ($Silent -or $LaunchApp) { return }
    if ($CheckOnly) { Write-Output $Text; return }
    Initialize-OPBSUpdaterUi
    [void][Windows.Forms.MessageBox]::Show($Text, 'Actualizaciones de Presenter Broadcast Studio', 'OK', 'Information')
}

function Show-ErrorMessage([string] $Text) {
    if ($Silent -or $LaunchApp) { return }
    if ($CheckOnly) { Write-Output "ERROR: $Text"; return }
    Initialize-OPBSUpdaterUi
    [void][Windows.Forms.MessageBox]::Show($Text, 'Actualizaciones de Presenter Broadcast Studio', 'OK', 'Error')
}

function ConvertTo-OPBSVersion([Parameter(Mandatory)][string] $Value) {
    $Normalized = $Value.Trim() -replace '^opbs-v', '' -replace '^v', ''
    if ($Normalized -notmatch '^(\d+\.\d+\.\d+)([A-Za-z][A-Za-z0-9.-]*)?$') {
        throw "La versión '$Value' no usa el formato mayor.menor.parche (con sufijo beta opcional)."
    }
    $Core = $Matches[1]
    $Suffix = [string]$Matches[2]
    $SortVersion = [version]("$Core." + $(if ($Suffix) { '1' } else { '0' }))
    return [pscustomobject]@{ Text = $Normalized; SortVersion = $SortVersion }
}

function Show-UpdatePrompt {
    param([string] $AvailableVersion, [string] $ReleaseNotes)
    Initialize-OPBSUpdaterUi
    $Form = New-Object Windows.Forms.Form
    $Form.Text = "Nueva versión disponible - Presenter Broadcast Studio $AvailableVersion"
    $Form.StartPosition = 'CenterScreen'
    $Form.Size = New-Object Drawing.Size(780, 660)
    $Form.MinimumSize = New-Object Drawing.Size(640, 500)
    $Form.ShowIcon = $false
    $Form.BackColor = [Drawing.Color]::FromArgb(24, 27, 34)
    $Form.ForeColor = [Drawing.Color]::White

    $Header = New-Object Windows.Forms.Label
    $Header.Dock = 'Top'; $Header.Height = 88
    $Header.Padding = New-Object Windows.Forms.Padding(18, 16, 18, 8)
    $Header.Font = New-Object Drawing.Font('Segoe UI', 12, [Drawing.FontStyle]::Bold)
    $Header.Text = "Presenter Broadcast Studio $AvailableVersion está disponible.`r`nLa actualización conservará tu biblioteca y configuración."
    $Form.Controls.Add($Header)

    $Notes = New-Object Windows.Forms.RichTextBox
    $Notes.Dock = 'Fill'; $Notes.ReadOnly = $true; $Notes.DetectUrls = $true
    $Notes.BackColor = [Drawing.Color]::FromArgb(17, 19, 24)
    $Notes.ForeColor = [Drawing.Color]::White
    $Notes.BorderStyle = 'FixedSingle'; $Notes.Font = New-Object Drawing.Font('Segoe UI', 9.5)
    $Notes.Text = if ([string]::IsNullOrWhiteSpace($ReleaseNotes)) {
        'Consulta las notas completas en el Release de OPBS en GitHub.'
    } else { $ReleaseNotes.Trim() }
    $Form.Controls.Add($Notes); $Notes.BringToFront()

    $Buttons = New-Object Windows.Forms.FlowLayoutPanel
    $Buttons.Dock = 'Bottom'; $Buttons.Height = 66
    $Buttons.Padding = New-Object Windows.Forms.Padding(8, 13, 14, 8)
    $Buttons.FlowDirection = 'RightToLeft'
    $Install = New-Object Windows.Forms.Button
    $Install.Text = 'Actualizar ahora'; $Install.AutoSize = $true; $Install.DialogResult = 'Yes'
    $Postpone = New-Object Windows.Forms.Button
    $Postpone.Text = 'Posponer'; $Postpone.AutoSize = $true; $Postpone.DialogResult = 'No'
    [void]$Buttons.Controls.Add($Install); [void]$Buttons.Controls.Add($Postpone)
    $Form.AcceptButton = $Install; $Form.CancelButton = $Postpone
    $Form.Controls.Add($Buttons); $Buttons.BringToFront()
    $Form.Add_Shown({ $Form.Activate() })
    return $Form.ShowDialog() -eq 'Yes'
}

function Stop-OPBSGracefully([int] $ProcessId) {
    if ($ProcessId -le 0) { return }
    $Process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if (-not $Process) { return }
    [void]$Process.CloseMainWindow()
    try { Wait-Process -Id $ProcessId -Timeout 20 -ErrorAction Stop } catch {
        Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $ProcessId -Timeout 5 -ErrorAction SilentlyContinue
    }
}

try {
    $ConfigurationPath = Join-Path $PSScriptRoot 'opbs-release.json'
    if (-not (Test-Path -LiteralPath $ConfigurationPath)) {
        throw 'No se encontró opbs-release.json junto al actualizador.'
    }
    $Configuration = Get-Content -Raw -LiteralPath $ConfigurationPath | ConvertFrom-Json
    $CurrentVersion = ConvertTo-OPBSVersion $(if ($CurrentVersionOverride) {
        $CurrentVersionOverride
    } else { [string]$Configuration.version })

    $Headers = @{
        Accept = 'application/vnd.github+json'
        'User-Agent' = "OPBS-Updater/$($CurrentVersion.Text)"
        'X-GitHub-Api-Version' = '2026-03-10'
    }

    if ($LocalReleaseDirectory) {
        $AvailableVersionText = [string]$Configuration.version
        $AvailableVersion = ConvertTo-OPBSVersion $AvailableVersionText
        $InstallerAsset = [PSCustomObject]@{
            name = [string]$Configuration.installerAsset
            browser_download_url = (Join-Path $LocalReleaseDirectory ([string]$Configuration.installerAsset))
        }
        $ChecksumAsset = [PSCustomObject]@{
            name = [string]$Configuration.checksumAsset
            browser_download_url = (Join-Path $LocalReleaseDirectory ([string]$Configuration.checksumAsset))
        }
        $ReleaseNotesPath = Join-Path $LocalReleaseDirectory 'release-notes.md'
        $ReleaseNotes = if (Test-Path -LiteralPath $ReleaseNotesPath) {
            Get-Content -Raw -LiteralPath $ReleaseNotesPath
        } else { "Prueba local de actualización a Presenter Broadcast Studio $($AvailableVersion.Text)." }
        $ReleaseUrl = $LocalReleaseDirectory
    } else {
        $Repository = [string]$Configuration.githubRepository
        if ($Repository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
            throw 'El repositorio de actualizaciones de OPBS todavía no está configurado.'
        }
        $ReleaseApiUrl = if ($ReleaseApiUrlOverride) {
            $ReleaseApiUrlOverride
        } else {
            "https://api.github.com/repos/$Repository/releases/latest"
        }
        $Release = Get-OPBSLatestRelease -Uri $ReleaseApiUrl -Headers $Headers `
            -TimeoutMilliseconds $MetadataTimeoutMilliseconds
        $AvailableVersion = ConvertTo-OPBSVersion ([string]$Release.tag_name)
        $InstallerAsset = $Release.assets | Where-Object name -eq $Configuration.installerAsset | Select-Object -First 1
        $ChecksumAsset = $Release.assets | Where-Object name -eq $Configuration.checksumAsset | Select-Object -First 1
        $ReleaseNotes = [string]$Release.body
        $ReleaseUrl = [string]$Release.html_url
    }

    if ($AvailableVersion.SortVersion -le $CurrentVersion.SortVersion) {
        Show-Information "Presenter Broadcast Studio $($CurrentVersion.Text) ya es la versión más reciente."
        Start-OPBS
        exit 0
    }
    if (-not $InstallerAsset -or -not $ChecksumAsset) {
        throw "La versión $($AvailableVersion.Text) no contiene el instalador y su SHA-256."
    }
    if ($CheckOnly) {
        [PSCustomObject]@{
            currentVersion = $CurrentVersion.Text
            availableVersion = $AvailableVersion.Text
            installerUrl = $InstallerAsset.browser_download_url
            releaseUrl = $ReleaseUrl
            releaseNotes = $ReleaseNotes
        } | ConvertTo-Json
        exit 0
    }
    if (-not (Show-UpdatePrompt -AvailableVersion $AvailableVersion.Text -ReleaseNotes $ReleaseNotes)) {
        Start-OPBS
        exit 0
    }

    $DownloadDirectory = Join-Path ([IO.Path]::GetTempPath()) "OPBS-Update-$($AvailableVersion.Text)"
    New-Item -ItemType Directory -Path $DownloadDirectory -Force | Out-Null
    $InstallerPath = Join-Path $DownloadDirectory ([string]$Configuration.installerAsset)
    $ChecksumPath = Join-Path $DownloadDirectory ([string]$Configuration.checksumAsset)
    if ($LocalReleaseDirectory) {
        Copy-Item -LiteralPath $InstallerAsset.browser_download_url -Destination $InstallerPath -Force
        Copy-Item -LiteralPath $ChecksumAsset.browser_download_url -Destination $ChecksumPath -Force
    } else {
        Invoke-WebRequest -Uri $InstallerAsset.browser_download_url -Headers $Headers -OutFile $InstallerPath
        Invoke-WebRequest -Uri $ChecksumAsset.browser_download_url -Headers $Headers -OutFile $ChecksumPath
    }

    $ExpectedHash = [regex]::Match((Get-Content -Raw -LiteralPath $ChecksumPath), '(?i)\b[0-9a-f]{64}\b').Value
    $Sha256 = [System.Security.Cryptography.SHA256]::Create()
    $InstallerStream = [System.IO.File]::OpenRead($InstallerPath)
    try {
        $ActualHash = ([System.BitConverter]::ToString($Sha256.ComputeHash($InstallerStream))).Replace('-', '')
    } finally {
        $InstallerStream.Dispose()
        $Sha256.Dispose()
    }
    if (-not $ExpectedHash -or $ActualHash -ne $ExpectedHash) {
        Remove-Item -LiteralPath $InstallerPath -Force -ErrorAction SilentlyContinue
        throw 'El instalador descargado no coincide con el SHA-256 publicado. La actualización fue cancelada.'
    }

    & (Join-Path $PSScriptRoot 'OPBS-MigrateData.ps1')
    Stop-OPBSGracefully $CurrentProcessId
    $InstallerArguments = @('/S')
    if ($InstallRootOverride) { $InstallerArguments += "/D=$InstallRootOverride" }
    $InstallerProcess = Start-Process -FilePath $InstallerPath -ArgumentList $InstallerArguments -Wait -PassThru
    if ($InstallerProcess.ExitCode -ne 0) {
        throw "El instalador terminó con el código $($InstallerProcess.ExitCode)."
    }
} catch {
    Show-ErrorMessage $_.Exception.Message
    if ($LaunchApp) { Start-OPBS; exit 0 }
    exit 1
}
