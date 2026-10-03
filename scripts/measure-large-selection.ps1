# Times the three commands on large selections, to catch changes that make the worker slow.
# Works only on fixtures created under %TEMP%\ExplorerMate.Tests and deletes them at the end.
# Runs the worker with --silent, so no dialog and no progress window appear and nothing is
# added to Explorer's undo history. Takes a few minutes.
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [int]$Items = 5000,
    [int]$FilesInFolder = 20000
)

. (Join-Path $PSScriptRoot 'Common.ps1')
Add-Type -Namespace ET -Name MeasureNative -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
public static extern uint GetLongPathNameW(string path, System.Text.StringBuilder buffer, uint size);
'@

$exe = Join-Path $RepoRoot "build\x64\$Configuration\ExplorerMate.exe"
if (-not (Test-Path $exe)) { throw "Build first: $exe not found." }
if ($Items -gt 5000) { throw 'A request holds at most 5000 items.' }

$requests = Join-Path $env:LOCALAPPDATA 'ExplorerMate\requests'
[void](New-Item -ItemType Directory -Force $requests)

# The worker only accepts long paths; %TEMP% is often a short 8.3 one.
$created = (New-Item -ItemType Directory -Force (Join-Path ([IO.Path]::GetTempPath()) ("ExplorerMate.Tests\measure-" + [guid]::NewGuid().ToString('N').Substring(0, 8)))).FullName
$buffer = New-Object System.Text.StringBuilder 1024
[void][ET.MeasureNative]::GetLongPathNameW($created, $buffer, 1024)
$root = $buffer.ToString()

function New-Files([string]$Folder, [int]$Count) {
    [void][IO.Directory]::CreateDirectory($Folder)
    $paths = New-Object 'System.Collections.Generic.List[string]'
    for ($i = 1; $i -le $Count; $i++) {
        $path = Join-Path $Folder "photo $i.txt"
        [IO.File]::WriteAllText($path, 'x')
        $paths.Add($path)
    }
    return , $paths
}

# Runs the worker on a request file, like the context menu does, and reports time and memory.
function Measure-Worker([string]$Label, [string]$Action, $Paths, [string[]]$Extra) {
    $file = Join-Path $requests ([guid]::NewGuid().ToString() + '.etreq')
    $lines = @('ExplorerMate-Request 1', "action=$Action") + @($Paths | ForEach-Object { "item=$_" })
    [IO.File]::WriteAllText($file, ($lines -join "`n") + "`n", (New-Object System.Text.UTF8Encoding $false))
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $process = Start-Process $exe -ArgumentList (@('--request', "`"$file`"", '--silent') + $Extra) -PassThru
    $null = $process.Handle
    $peak = 0
    while (-not $process.HasExited) {
        $process.Refresh()
        try { $peak = [Math]::Max($peak, $process.PeakWorkingSet64) } catch { }
        Start-Sleep -Milliseconds 100
    }
    $watch.Stop()
    Write-Host ('{0,-34} {1,7:N1} s   peak {2,6:N0} MB   exit {3}' -f $Label, $watch.Elapsed.TotalSeconds, ($peak / 1MB), $process.ExitCode)
}

try {
    Measure-Worker "Bulk rename, $Items files" 'rename' (New-Files (Join-Path $root 'rename') $Items) @('--mask', '[N]_[C]')
    Measure-Worker "New folder, $Items files" 'group' (New-Files (Join-Path $root 'group') $Items) @('--name', 'Grouped')
    Measure-Worker "Duplicate, $Items files" 'duplicate' (New-Files (Join-Path $root 'duplicate') $Items) @()
    $album = Join-Path $root 'folder\Album'
    [void](New-Files $album $FilesInFolder)
    Measure-Worker "Duplicate, 1 folder of $FilesInFolder" 'duplicate' @($album) @()
}
finally {
    if ($root -like '*\ExplorerMate.Tests\measure-*') { Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue }
}
