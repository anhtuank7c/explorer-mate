# Produces the Store listing screenshots that can be made without a person:
#   1. creates a demo folder with harmless sample files (build\store-demo\Da Lat 2026),
#   2. opens each Explorer Mate dialog on those files, captures it and cancels it
#      (nothing is renamed or moved),
#   3. places every capture on a 1920x1080 canvas with a caption.
# Output: packaging\store\screenshots\*.png
#
# The context menu and the tray menu only appear on a real click; capture those by hand in
# the demo folder (Win+Shift+S) - see docs\RELEASING.md.
[CmdletBinding()]
param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')

. (Join-Path $PSScriptRoot 'Common.ps1')
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class StoreShot {
    public delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr dialog, int id);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window, out RECT rect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr window, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern IntPtr SendMessageW(IntPtr window, uint message, IntPtr wParam, string lParam);
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
    [DllImport("dwmapi.dll")] static extern int DwmGetWindowAttribute(IntPtr window, int attribute, out RECT value, int size);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }

    public static IntPtr FindWindowOf(uint processId) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((window, parameter) => {
            uint owner; GetWindowThreadProcessId(window, out owner);
            if (owner == processId && IsWindowVisible(window)) { found = window; return false; }
            return true;
        }, IntPtr.Zero);
        return found;
    }
    public static void SetText(IntPtr window, string text) { SendMessageW(window, 0x000C, IntPtr.Zero, text); }
    // The visible frame without the invisible resize border around it.
    public static RECT VisibleBounds(IntPtr window) {
        RECT rect;
        if (DwmGetWindowAttribute(window, 9 /* DWMWA_EXTENDED_FRAME_BOUNDS */, out rect, Marshal.SizeOf(typeof(RECT))) != 0) {
            GetWindowRect(window, out rect);
        }
        return rect;
    }
}
'@
# Per-monitor DPI awareness for this thread, so window sizes are real pixels.
[void][StoreShot]::SetThreadDpiAwarenessContext([IntPtr](-4))

$exe = Join-Path $RepoRoot "build\x64\$Configuration\ExplorerMate.exe"
if (-not (Test-Path $exe)) { throw "Build first: $exe not found." }
$outDir = Join-Path $RepoRoot 'packaging\store\screenshots'
New-Item -ItemType Directory -Force $outDir | Out-Null

# --- Demo folder ---------------------------------------------------------------------------
$demo = Join-Path $RepoRoot 'build\store-demo\Da Lat 2026'
New-Item -ItemType Directory -Force $demo | Out-Null
New-Item -ItemType Directory -Force (Join-Path $demo 'Receipts') | Out-Null
$palette = @('#F4A261', '#2A9D8F', '#E76F51', '#457B9D', '#8AB17D', '#E9C46A', '#6D597A', '#3A86FF', '#B56576', '#52B788', '#F28482', '#577590')
for ($i = 0; $i -lt $palette.Count; $i++) {
    $path = Join-Path $demo ('IMG_{0:D4}.jpg' -f (2041 + $i))
    if (Test-Path $path) { continue }
    $bitmap = New-Object System.Drawing.Bitmap 1200, 800
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = 'AntiAlias'
    $color = [System.Drawing.ColorTranslator]::FromHtml($palette[$i])
    $sky = New-Object System.Drawing.Drawing2D.LinearGradientBrush (New-Object System.Drawing.Point 0, 0), (New-Object System.Drawing.Point 0, 800), ([System.Drawing.Color]::FromArgb(255, 245, 235)), $color
    $graphics.FillRectangle($sky, 0, 0, 1200, 800)
    $graphics.FillEllipse([System.Drawing.Brushes]::White, 820 - 40 * ($i % 5), 110 + 20 * ($i % 3), 150, 150)
    $hill = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(150, 40, 60, 70))
    $graphics.FillPolygon($hill, @((New-Object System.Drawing.Point 0, 800), (New-Object System.Drawing.Point (300 + 40 * ($i % 4)), 420), (New-Object System.Drawing.Point 700, 800)))
    $graphics.FillPolygon($hill, @((New-Object System.Drawing.Point 450, 800), (New-Object System.Drawing.Point (850 - 30 * ($i % 3)), 500), (New-Object System.Drawing.Point 1200, 800)))
    $graphics.Dispose()
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Jpeg)
    $bitmap.Dispose()
}
Set-Content (Join-Path $demo 'Itinerary.txt') 'Day 1: flower garden. Day 2: pine hills.'
Set-Content (Join-Path $demo 'Packing list.txt') 'Jacket, camera, charger.'
$photos = @(Get-ChildItem $demo -Filter 'IMG_*.jpg' | Sort-Object Name | ForEach-Object FullName)

# --- Capture -------------------------------------------------------------------------------
# Starts the worker with a dialog, optionally types into controls, captures the dialog and
# cancels it. Nothing on disk changes.
function Get-DialogCapture([string[]]$Arguments, [hashtable]$SetText = @{}) {
    $quoted = $Arguments | ForEach-Object { if ($_ -match '\s') { '"' + $_ + '"' } else { $_ } }
    $process = Start-Process $exe -ArgumentList $quoted -PassThru
    $dialog = [IntPtr]::Zero
    foreach ($attempt in 1..60) {
        Start-Sleep -Milliseconds 100
        $dialog = [StoreShot]::FindWindowOf($process.Id)
        if ($dialog -ne [IntPtr]::Zero) { break }
    }
    if ($dialog -eq [IntPtr]::Zero) { if (-not $process.HasExited) { $process.Kill() }; throw "No dialog appeared for: $Arguments" }
    foreach ($id in $SetText.Keys) { [StoreShot]::SetText([StoreShot]::GetDlgItem($dialog, [int]$id), [string]$SetText[$id]) }
    Start-Sleep -Milliseconds 700

    $outer = New-Object StoreShot+RECT; [void][StoreShot]::GetWindowRect($dialog, [ref]$outer)
    $visible = [StoreShot]::VisibleBounds($dialog)
    $full = New-Object System.Drawing.Bitmap ($outer.Right - $outer.Left), ($outer.Bottom - $outer.Top)
    $graphics = [System.Drawing.Graphics]::FromImage($full)
    $dc = $graphics.GetHdc(); [void][StoreShot]::PrintWindow($dialog, $dc, 2); $graphics.ReleaseHdc($dc); $graphics.Dispose()
    # PrintWindow paints the window's one-pixel frame black; leave it out.
    $frame = 2
    $crop = New-Object System.Drawing.Rectangle ($visible.Left - $outer.Left + $frame), ($visible.Top - $outer.Top), ($visible.Right - $visible.Left - 2 * $frame), ($visible.Bottom - $visible.Top - $frame)
    $capture = $full.Clone($crop, $full.PixelFormat); $full.Dispose()

    [void][StoreShot]::PostMessageW($dialog, 0x0111, [IntPtr]2, [IntPtr]::Zero)   # Cancel
    if (-not $process.WaitForExit(10000)) { $process.Kill() }
    return $capture
}

# Places a capture on a 1920x1080 canvas under a caption.
function Save-StoreScreenshot([System.Drawing.Bitmap]$Capture, [string]$Caption, [string]$SubCaption, [string]$FileName) {
    $canvas = New-Object System.Drawing.Bitmap 1920, 1080
    $graphics = [System.Drawing.Graphics]::FromImage($canvas)
    $graphics.SmoothingMode = 'AntiAlias'; $graphics.TextRenderingHint = 'AntiAliasGridFit'; $graphics.InterpolationMode = 'HighQualityBicubic'
    $background = New-Object System.Drawing.Drawing2D.LinearGradientBrush (New-Object System.Drawing.Point 0, 0), (New-Object System.Drawing.Point 1920, 1080), ([System.Drawing.Color]::FromArgb(232, 241, 252)), ([System.Drawing.Color]::FromArgb(255, 246, 221))
    $graphics.FillRectangle($background, 0, 0, 1920, 1080)

    # Sizes in pixels, so the layout does not depend on the display scaling of this machine.
    $pixel = [System.Drawing.GraphicsUnit]::Pixel
    $titleFont = New-Object System.Drawing.Font 'Segoe UI Semibold', 60, ([System.Drawing.FontStyle]::Regular), $pixel
    $subFont = New-Object System.Drawing.Font 'Segoe UI', 30, ([System.Drawing.FontStyle]::Regular), $pixel
    $centered = New-Object System.Drawing.StringFormat; $centered.Alignment = 'Center'
    $graphics.DrawString($Caption, $titleFont, (New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(27, 27, 27))), (New-Object System.Drawing.RectangleF 0, 48, 1920, 96), $centered)
    $graphics.DrawString($SubCaption, $subFont, (New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(70, 70, 70))), (New-Object System.Drawing.RectangleF 0, 142, 1920, 56), $centered)

    # Fit the capture into the area below the caption without enlarging it beyond 1.5x.
    $scale = [Math]::Min(1.5, [Math]::Min(1700 / $Capture.Width, 780 / $Capture.Height))
    $width = [int]($Capture.Width * $scale); $height = [int]($Capture.Height * $scale)
    $x = [int]((1920 - $width) / 2); $y = 230 + [int]((780 - $height) / 2)
    foreach ($spread in 18, 12, 6) {
        $shadow = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(14, 0, 0, 0))
        $graphics.FillRectangle($shadow, $x - $spread, $y - $spread + 10, $width + 2 * $spread, $height + 2 * $spread)
    }
    $graphics.DrawImage($Capture, $x, $y, $width, $height)
    $graphics.DrawRectangle((New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(190, 190, 190)), 1), $x, $y, $width - 1, $height - 1)
    $graphics.Dispose()
    $canvas.Save((Join-Path $outDir $FileName), [System.Drawing.Imaging.ImageFormat]::Png)
    $canvas.Dispose(); $Capture.Dispose()
    Write-Host "Wrote $FileName"
}

$rename = Get-DialogCapture (@('--action', 'rename') + $photos) @{ 1003 = 'Da Lat [C]' }
Save-StoreScreenshot $rename 'Rename many files at once' 'Name masks, a counter, search and replace - with a live preview before anything changes' '02-bulk-rename.png'

$group = Get-DialogCapture (@('--action', 'group') + $photos[0..3]) @{ 1001 = 'Day 1 - Flower garden' }
Save-StoreScreenshot $group 'New folder with selection' 'Select files, name the folder, and they move into it' '03-new-folder.png'

$about = Get-DialogCapture @('--about')
Save-StoreScreenshot $about 'Three commands, right where you work' 'In the right-click menu of File Explorer, and as keyboard shortcuts' '04-about.png'

Write-Host "Demo folder for the hand-made captures: $demo"
