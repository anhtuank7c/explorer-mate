# Regenerates every icon file from the two SVG masters in packaging\icon:
#   ExplorerMate.svg        the icon, used for 32 px and larger
#   ExplorerMate-small.svg  simplified drawing for 16-24 px
# Outputs (committed to the repository, so a normal build does not need this script):
#   src\App\ExplorerMate.ico            EXE, window and tray icon
#   packaging\Assets\*.png              package logos, including the size-specific variants
#   packaging\store\StoreLogo-300.png   square logo for the Store listing
#
# The SVGs are rendered with Microsoft Edge in headless mode, so the PNGs are exactly what a
# browser shows for the master files. Run this again after editing an SVG.
[CmdletBinding()]
param()

. (Join-Path $PSScriptRoot 'Common.ps1')
Add-Type -AssemblyName System.Drawing

$edge = @("${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
          "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $edge) { throw 'Microsoft Edge not found; it is needed to render the SVG masters.' }

$iconDir = Join-Path $RepoRoot 'packaging\icon'
$assetDir = Join-Path $RepoRoot 'packaging\Assets'
$storeDir = Join-Path $RepoRoot 'packaging\store'
$workDir = Join-Path $RepoRoot 'build\icons'
foreach ($dir in $assetDir, $storeDir, $workDir) { New-Item -ItemType Directory -Force $dir | Out-Null }

$mainSvg = Get-Content (Join-Path $iconDir 'ExplorerMate.svg') -Raw
$smallSvg = Get-Content (Join-Path $iconDir 'ExplorerMate-small.svg') -Raw
$SmallDrawingMaxSize = 24

# Renders the application icon at $Size px, from the master meant for that size.
function New-IconBitmap([int]$Size) {
    $svg = if ($Size -le $SmallDrawingMaxSize) { $smallSvg } else { $mainSvg }
    return New-SvgBitmap $svg $Size
}

# Renders SVG text at $Size px into a transparent bitmap. Edge cannot open a window as small
# as an icon, so the SVG is drawn in the corner of a larger page and cropped.
function New-SvgBitmap([string]$Svg, [int]$Size) {
    $page = Join-Path $workDir "render-$Size.html"
    $shot = Join-Path $workDir "render-$Size.png"
    $html = "<!doctype html><html><head><meta charset='utf-8'><style>html,body{margin:0;background:transparent}svg{display:block;width:${Size}px;height:${Size}px}</style></head><body>$Svg</body></html>"
    [IO.File]::WriteAllText($page, $html)
    if (Test-Path $shot) { Remove-Item $shot }
    $window = [Math]::Max(600, $Size)
    # Start-Process, not "&": Edge reports progress on stderr, which Windows PowerShell would
    # turn into a terminating error under $ErrorActionPreference = 'Stop'.
    $arguments = @('--headless=new', '--disable-gpu', '--hide-scrollbars', '--force-device-scale-factor=1',
        '--default-background-color=00000000', "--window-size=$window,$window", "--screenshot=`"$shot`"",
        ('"file:///' + $page.Replace('\', '/') + '"'))
    Start-Process $edge -ArgumentList $arguments -Wait -WindowStyle Hidden `
        -RedirectStandardError (Join-Path $workDir 'edge-stderr.txt')
    foreach ($attempt in 1..50) { if (Test-Path $shot) { break }; Start-Sleep -Milliseconds 100 }
    if (-not (Test-Path $shot)) { throw "Edge did not render the $Size px icon." }

    $full = [System.Drawing.Bitmap]::FromFile($shot)
    try {
        $icon = New-Object System.Drawing.Bitmap $Size, $Size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [System.Drawing.Graphics]::FromImage($icon)
        $graphics.DrawImage($full, (New-Object System.Drawing.Rectangle 0, 0, $Size, $Size),
            (New-Object System.Drawing.Rectangle 0, 0, $Size, $Size), [System.Drawing.GraphicsUnit]::Pixel)
        $graphics.Dispose()
        return $icon
    } finally { $full.Dispose() }
}

function Save-Png([System.Drawing.Bitmap]$Bitmap, [string]$Path) {
    $Bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
}

# A logo of $Canvas px with the icon drawn at $IconSize px in the middle (transparent margin).
function Save-PaddedLogo([int]$Canvas, [int]$IconSize, [string]$Path) {
    $icon = New-IconBitmap $IconSize
    $logo = New-Object System.Drawing.Bitmap $Canvas, $Canvas, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($logo)
    $offset = [int](($Canvas - $IconSize) / 2)
    $graphics.DrawImage($icon, $offset, $offset, $IconSize, $IconSize)
    $graphics.Dispose(); $icon.Dispose()
    Save-Png $logo $Path
    $logo.Dispose()
}

function Save-Icon([int]$Size, [string]$Path) {
    $icon = New-IconBitmap $Size
    Save-Png $icon $Path
    $icon.Dispose()
}

# --- Package logos -------------------------------------------------------------------------
Get-ChildItem $assetDir -Filter *.png | Remove-Item
Save-Icon 44 (Join-Path $assetDir 'Square44x44Logo.png')
Save-Icon 88 (Join-Path $assetDir 'Square44x44Logo.scale-200.png')
# "targetsize" files are used for the taskbar, Start list and title bars; "unplated" means
# Windows draws them without a coloured backplate, on both dark and light themes.
foreach ($size in 16, 24, 32, 48, 256) {
    Save-Icon $size (Join-Path $assetDir "Square44x44Logo.targetsize-$size.png")
    Save-Icon $size (Join-Path $assetDir "Square44x44Logo.targetsize-${size}_altform-unplated.png")
    Save-Icon $size (Join-Path $assetDir "Square44x44Logo.targetsize-${size}_altform-lightunplated.png")
}
Save-PaddedLogo 150 96 (Join-Path $assetDir 'Square150x150Logo.png')
Save-PaddedLogo 300 192 (Join-Path $assetDir 'Square150x150Logo.scale-200.png')
Save-Icon 50 (Join-Path $assetDir 'StoreLogo.png')
Save-Icon 100 (Join-Path $assetDir 'StoreLogo.scale-200.png')
Save-PaddedLogo 300 240 (Join-Path $storeDir 'StoreLogo-300.png')

# --- ICO files (PNG-compressed entries) ----------------------------------------------------
# $Render is called with a size and returns the bitmap for it.
function Save-Ico([int[]]$Sizes, [scriptblock]$Render, [string]$Path) {
    $images = foreach ($size in $Sizes) {
        $bitmap = & $Render $size
        $stream = New-Object IO.MemoryStream
        $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
        $bitmap.Dispose()
        , $stream.ToArray()
    }
    $ico = New-Object IO.MemoryStream
    $writer = New-Object IO.BinaryWriter $ico
    $writer.Write([uint16]0); $writer.Write([uint16]1); $writer.Write([uint16]$Sizes.Count)
    $offset = 6 + 16 * $Sizes.Count
    for ($i = 0; $i -lt $Sizes.Count; $i++) {
        $dimension = if ($Sizes[$i] -ge 256) { 0 } else { $Sizes[$i] }   # 0 means 256
        $writer.Write([byte]$dimension); $writer.Write([byte]$dimension)
        $writer.Write([byte]0); $writer.Write([byte]0)
        $writer.Write([uint16]1); $writer.Write([uint16]32)
        $writer.Write([uint32]$images[$i].Length); $writer.Write([uint32]$offset)
        $offset += $images[$i].Length
    }
    foreach ($image in $images) { $writer.Write($image) }
    $writer.Flush()
    [IO.File]::WriteAllBytes($Path, $ico.ToArray())
}

$icoSizes = 16, 20, 24, 32, 40, 48, 64, 256
Save-Ico $icoSizes { param($size) New-IconBitmap $size } (Join-Path $RepoRoot 'src\App\ExplorerMate.ico')

# --- Context-menu command icons (Lucide, ISC license; see THIRD_PARTY_NOTICES.md) ----------
# Line icons drawn in "currentColor". The menu does not tint icons, so each one is rendered
# twice: dark strokes for the light theme, white strokes for the dark theme. The sizes are
# the menu's icon size at 100, 125, 150 and 200 % display scaling.
$menuDir = Join-Path $iconDir 'menu'
$menuOut = Join-Path $RepoRoot 'src\ShellExtension\icons'
New-Item -ItemType Directory -Force $menuOut | Out-Null
$menuIcons = @{ 'folder-plus' = 'group'; 'pen-line' = 'rename'; 'copy' = 'duplicate' }
$themes = @{ 'light' = '#1B1B1B'; 'dark' = '#FFFFFF' }
foreach ($source in $menuIcons.Keys) {
    $svg = Get-Content (Join-Path $menuDir "$source.svg") -Raw
    foreach ($theme in $themes.Keys) {
        $script:tintedSvg = $svg.Replace('currentColor', $themes[$theme])
        Save-Ico @(16, 20, 24, 32) { param($size) New-SvgBitmap $script:tintedSvg $size } `
            (Join-Path $menuOut "$($menuIcons[$source])-$theme.ico")
    }
}

Write-Host "Wrote $((Get-ChildItem $assetDir -Filter *.png).Count) package logos, the Store logo, src\App\ExplorerMate.ico and $((Get-ChildItem $menuOut -Filter *.ico).Count) menu icons."
