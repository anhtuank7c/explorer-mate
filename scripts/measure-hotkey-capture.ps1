# Times how long a keyboard shortcut waits for the selection to be read while other Explorer
# windows hold large selections. Opens Explorer windows on fixture folders under
# %TEMP%\ExplorerMate.Tests, selects every item in each, reads the diagnostics, then closes
# the windows and deletes the fixtures. Do not touch the keyboard or mouse while it runs:
# the last window must keep the focus.
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [int]$Windows = 3,
    [int]$ItemsPerWindow = 2000
)

. (Join-Path $PSScriptRoot 'Common.ps1')
Add-Type -Namespace ET -Name HotkeyNative -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("user32.dll")]
public static extern bool PostMessageW(System.IntPtr window, uint message, System.IntPtr wParam, System.IntPtr lParam);
[System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
public static extern uint GetLongPathNameW(string path, System.Text.StringBuilder buffer, uint size);
'@

$exe = Join-Path $RepoRoot "build\x64\$Configuration\ExplorerMate.exe"
if (-not (Test-Path $exe)) { throw "Build first: $exe not found." }

$created = (New-Item -ItemType Directory -Force (Join-Path ([IO.Path]::GetTempPath()) ("ExplorerMate.Tests\hotkey-" + [guid]::NewGuid().ToString('N').Substring(0, 8)))).FullName
$buffer = New-Object System.Text.StringBuilder 1024
[void][ET.HotkeyNative]::GetLongPathNameW($created, $buffer, 1024)
$root = $buffer.ToString()

function Wait-Until([scriptblock]$Condition, [string]$What, [int]$Seconds = 20) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $deadline) { $value = & $Condition; if ($null -ne $value -and $value -ne $false) { return $value }; Start-Sleep -Milliseconds 200 }
    throw "Timed out waiting for: $What"
}

$shell = New-Object -ComObject Shell.Application
$frames = @()
try {
    foreach ($number in 1..$Windows) {
        $folder = Join-Path $root "window $number"
        [void][IO.Directory]::CreateDirectory($folder)
        foreach ($i in 1..$ItemsPerWindow) { [IO.File]::WriteAllText((Join-Path $folder "photo $i.txt"), 'x') }

        Start-Process explorer.exe -ArgumentList "`"$folder`""
        $window = Wait-Until { $shell.Windows() | Where-Object { $_ -and $(try { $_.Document.Folder.Self.Path } catch { $null }) -eq $folder } | Select-Object -First 1 } "the Explorer window on $folder"
        $frames += [long]$window.HWND
        Wait-Until { $window.Document.Folder.Items().Count -eq $ItemsPerWindow } 'the files to be listed' | Out-Null
        foreach ($item in $window.Document.Folder.Items()) { $window.Document.SelectItem($item, 1) }
        Write-Host "Window $number`: $($window.Document.SelectedItems().Count) of $ItemsPerWindow items selected"
    }

    $lines = @(& $exe --diagnose-explorer | ForEach-Object { $_ })
    $shortcut = @($lines | Where-Object { $_ -like 'shortcut *' })[0]
    Write-Host ($lines[0])
    Write-Host ($shortcut -replace '\[.*$', '')
    Write-Host (@($lines | Where-Object { $_ -like 'tabs=*' })[0])
}
finally {
    foreach ($frame in $frames) { [void][ET.HotkeyNative]::PostMessageW([IntPtr]$frame, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) }
    Start-Sleep -Milliseconds 1000
    if ($root -like '*\ExplorerMate.Tests\hotkey-*') { Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue }
}
