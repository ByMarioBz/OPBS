[CmdletBinding()]
param(
	[string] $Source = (Join-Path (Split-Path -Parent $PSScriptRoot) 'branding/presenter-broadcast-studio/PresenterBroadcastStudio-logo-user-glass-1000.png'),
	[string] $IconOutput = (Join-Path (Split-Path -Parent $PSScriptRoot) 'frontend/cmake/windows/obs-studio.ico'),
	[string[]] $PngOutputs = @(
		(Join-Path (Split-Path -Parent $PSScriptRoot) 'frontend/forms/images/obs.png'),
		(Join-Path (Split-Path -Parent $PSScriptRoot) 'frontend/forms/images/obs_paused.png'),
		(Join-Path (Split-Path -Parent $PSScriptRoot) 'frontend/forms/images/tray_active.png'),
		(Join-Path (Split-Path -Parent $PSScriptRoot) 'frontend/forms/images/active.png'),
		(Join-Path (Split-Path -Parent $PSScriptRoot) 'frontend/forms/images/paused.png')
	)
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$sourceImage = [System.Drawing.Image]::FromFile((Resolve-Path -LiteralPath $Source))
try {
	$variants = foreach ($size in @(16, 24, 32, 48, 64, 128, 256)) {
		$bitmap = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
		try {
			$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
			try {
				$graphics.Clear([System.Drawing.Color]::Transparent)
				$graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
				$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
				$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
				$graphics.DrawImage($sourceImage, 0, 0, $size, $size)
			} finally {
				$graphics.Dispose()
			}
			$stream = New-Object System.IO.MemoryStream
			try {
				$bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
				[pscustomobject]@{ Size = $size; Bytes = $stream.ToArray() }
			} finally {
				$stream.Dispose()
			}
		} finally {
			$bitmap.Dispose()
		}
	}

	foreach ($output in $PngOutputs) {
		$outputPath = [System.IO.Path]::GetFullPath($output)
		[System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($outputPath)) | Out-Null
		[System.IO.File]::WriteAllBytes($outputPath, $variants[-1].Bytes)
	}

	$iconPath = [System.IO.Path]::GetFullPath($IconOutput)
	[System.IO.Directory]::CreateDirectory([System.IO.Path]::GetDirectoryName($iconPath)) | Out-Null
	$iconStream = New-Object System.IO.MemoryStream
	try {
		$writer = New-Object System.IO.BinaryWriter($iconStream)
		try {
			$writer.Write([uint16]0)
			$writer.Write([uint16]1)
			$writer.Write([uint16]$variants.Count)
			$offset = 6 + (16 * $variants.Count)
			foreach ($variant in $variants) {
				$dimension = if ($variant.Size -eq 256) { [byte]0 } else { [byte]$variant.Size }
				$writer.Write($dimension)
				$writer.Write($dimension)
				$writer.Write([byte]0)
				$writer.Write([byte]0)
				$writer.Write([uint16]1)
				$writer.Write([uint16]32)
				$writer.Write([uint32]$variant.Bytes.Length)
				$writer.Write([uint32]$offset)
				$offset += $variant.Bytes.Length
			}
			foreach ($variant in $variants) {
				$writer.Write($variant.Bytes)
			}
			$writer.Flush()
			[System.IO.File]::WriteAllBytes($iconPath, $iconStream.ToArray())
		} finally {
			$writer.Dispose()
		}
	} finally {
		$iconStream.Dispose()
	}
} finally {
	$sourceImage.Dispose()
}

Write-Host "Icono OPBS generado: $IconOutput"
