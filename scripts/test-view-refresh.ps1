# Checks that File Explorer shows the result of a command without a manual refresh (F5).
# Opens ONE Explorer window on a fixture folder under %TEMP%, runs "New folder with
# selection" on two of its three files through the worker, and compares what the open file
# list shows with what is on disk. The window is closed at the end.
[CmdletBinding()]
param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')

. (Join-Path $PSScriptRoot 'Common.ps1')
Add-Type -Namespace ET -Name RefreshNative -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("user32.dll")]
public static extern bool PostMessageW(System.IntPtr window, uint message, System.IntPtr wParam, System.IntPtr lParam);
'@

$exe = Join-Path $RepoRoot "build\x64\$Configuration\ExplorerMate.exe"
if (-not (Test-Path $exe)) { throw "Build first: $exe not found." }

$root = Join-Path ([IO.Path]::GetTempPath()) ("ExplorerMate.Tests\refresh-" + [guid]::NewGuid().ToString('N').Substring(0, 8))
$folder = (Get-Item (New-Item -ItemType Directory -Force $root).FullName).FullName
foreach ($name in 'a.txt', 'b.txt', 'c.txt') { Set-Content (Join-Path $folder $name) 'x' }

# Number of items the Explorer tab showing $folder currently displays, from the diagnostics.
function Get-ItemsInView {
    $lines = @(& $exe --diagnose-explorer | ForEach-Object { $_ })
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -eq "  folder=$folder" -and $lines[$i + 1] -match 'itemsInView=(-?\d+)') { return [int]$Matches[1] }
    }
    return $null
}
function Wait-Until([scriptblock]$Condition, [string]$What, [int]$Seconds = 15) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $deadline) { $value = & $Condition; if ($null -ne $value -and $value -ne $false) { return $value }; Start-Sleep -Milliseconds 200 }
    throw "Timed out waiting for: $What"
}

$shell = New-Object -ComObject Shell.Application
$frame = 0
$failure = $null
try {
    Start-Process explorer.exe -ArgumentList "`"$folder`""
    $window = Wait-Until { $shell.Windows() | Where-Object { $_ -and $(try { $_.Document.Folder.Self.Path } catch { $null }) -eq $folder } | Select-Object -First 1 } 'the Explorer window'
    $frame = [long]$window.HWND
    Wait-Until { (Get-ItemsInView) -eq 3 } 'the three files to be listed' | Out-Null

    & $exe --action group --name 'Grouped' (Join-Path $folder 'a.txt') (Join-Path $folder 'b.txt') | Out-Null
    $onDisk = @(Get-ChildItem $folder).Name -join ', '
    Write-Host "On disk after the command: $onDisk"

    # Give Explorer a moment to react on its own; nothing here refreshes the window.
    $shown = $null
    foreach ($attempt in 1..25) { Start-Sleep -Milliseconds 200; $shown = Get-ItemsInView; if ($shown -eq 2) { break } }
    Write-Host "Items shown by the open window: $shown (expected 2: the new folder and c.txt)"
    if ($shown -ne 2) { $failure = "The open window shows $shown item(s) instead of 2; it needs a manual refresh." }
}
finally {
    if ($frame -ne 0) { [void][ET.RefreshNative]::PostMessageW([IntPtr]$frame, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero); Start-Sleep -Milliseconds 800 }
    if ($root -like '*\ExplorerMate.Tests\refresh-*') { Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue }
}
if ($failure) { throw $failure }
Write-Host 'PASS: the window showed the result without a refresh.'
