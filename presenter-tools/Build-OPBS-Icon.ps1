[CmdletBinding()]
param(
    [string] $PrimarySource,
    [string] $SecondarySource,
    [string] $IconOutput,
    [string] $SecondaryIconOutput,
    [string] $SecondaryPngOutput,
    [string[]] $PngOutputs
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$ProjectRoot = Split-Path -Parent $PSScriptRoot
if (-not $PrimarySource) { $PrimarySource = Join-Path $ProjectRoot 'branding/presenter-broadcast-studio/PresenterBroadcastStudio-logo-glass-t2-1000.png' }
if (-not $SecondarySource) { $SecondarySource = Join-Path $ProjectRoot 'branding/presenter-broadcast-studio/PresenterBroadcastStudio-logo-mglass-1000.png' }
if (-not $IconOutput) { $IconOutput = Join-Path $ProjectRoot 'frontend/cmake/windows/obs-studio.ico' }
if (-not $SecondaryIconOutput) { $SecondaryIconOutput = Join-Path $ProjectRoot 'frontend/cmake/windows/obs-studio-mglass.ico' }
if (-not $SecondaryPngOutput) { $SecondaryPngOutput = Join-Path $ProjectRoot 'frontend/forms/images/opbs_about.png' }
if (-not $PngOutputs) {
    $PngOutputs = @(
        (Join-Path $ProjectRoot 'frontend/forms/images/obs.png'),
        (Join-Path $ProjectRoot 'frontend/forms/images/obs_paused.png'),
        (Join-Path $ProjectRoot 'frontend/forms/images/tray_active.png'),
        (Join-Path $ProjectRoot 'frontend/forms/images/active.png'),
        (Join-Path $ProjectRoot 'frontend/forms/images/paused.png')
    )
}

function New-PngVariants([string] $SourcePath) {
    $sourceImage = [System.Drawing.Image]::FromFile((Resolve-Path -LiteralPath $SourcePath))
    try {
        return @(foreach ($size in @(16, 24, 32, 48, 64, 128, 256)) {
            $bitmap = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            try {
                $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
                try {
                    $graphics.Clear([System.Drawing.Color]::Transparent)
                    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                    $graphics.DrawImage($sourceImage, 0, 0, $size, $size)
                } finally { $graphics.Dispose() }
                $stream = New-Object System.IO.MemoryStream
                try {
                    $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
                    [pscustomobject]@{ Size = $size; Bytes = $stream.ToArray() }
                } finally { $stream.Dispose() }
            } finally { $bitmap.Dispose() }
        })
    } finally { $sourceImage.Dispose() }
}

function Write-Png([string] $OutputPath, [byte[]] $Bytes) {
    $fullPath = [System.IO.Path]::GetFullPath($OutputPath)
    [System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($fullPath)) | Out-Null
    [System.IO.File]::WriteAllBytes($fullPath, $Bytes)
}

function Write-Ico([string] $OutputPath, $Variants) {
    $fullPath = [System.IO.Path]::GetFullPath($OutputPath)
    [System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($fullPath)) | Out-Null
    $iconStream = New-Object System.IO.MemoryStream
    try {
        $writer = New-Object System.IO.BinaryWriter($iconStream)
        try {
            $writer.Write([uint16]0); $writer.Write([uint16]1); $writer.Write([uint16]$Variants.Count)
            $offset = 6 + (16 * $Variants.Count)
            foreach ($variant in $Variants) {
                $dimension = if ($variant.Size -eq 256) { [byte]0 } else { [byte]$variant.Size }
                $writer.Write($dimension); $writer.Write($dimension); $writer.Write([byte]0); $writer.Write([byte]0)
                $writer.Write([uint16]1); $writer.Write([uint16]32); $writer.Write([uint32]$variant.Bytes.Length); $writer.Write([uint32]$offset)
                $offset += $variant.Bytes.Length
            }
            foreach ($variant in $Variants) { $writer.Write($variant.Bytes) }
            $writer.Flush(); [System.IO.File]::WriteAllBytes($fullPath, $iconStream.ToArray())
        } finally { $writer.Dispose() }
    } finally { $iconStream.Dispose() }
}

$primaryVariants = New-PngVariants $PrimarySource
$secondaryVariants = New-PngVariants $SecondarySource
foreach ($output in $PngOutputs) { Write-Png $output $primaryVariants[-1].Bytes }
Write-Png $SecondaryPngOutput $secondaryVariants[-1].Bytes
Write-Ico $IconOutput $primaryVariants
Write-Ico $SecondaryIconOutput $secondaryVariants
Write-Host "Iconos OPBS generados: $IconOutput y $SecondaryIconOutput"
