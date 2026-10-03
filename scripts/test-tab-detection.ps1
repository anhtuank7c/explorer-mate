# Verifies that ExplorerMate identifies the active File Explorer tab and its selection.
# Opens ONE new Explorer window on fixture folders under %TEMP%, adds two more tabs (one on a
# different folder, one on the same folder with a different selection), activates each tab in
# turn and compares what `ExplorerMate.exe --diagnose-explorer` reports with what was set up.
# Other Explorer windows are not touched. The window is closed at the end.
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$KeepWindow
)

. (Join-Path $PSScriptRoot 'Common.ps1')
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
Add-Type -Namespace ET -Name Native -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("user32.dll")]
public static extern bool PostMessageW(System.IntPtr window, uint message, System.IntPtr wParam, System.IntPtr lParam);
'@

$exe = Join-Path $RepoRoot "build\x64\$Configuration\ExplorerMate.exe"
if (-not (Test-Path $exe)) { throw "Build first: $exe not found." }

# --- Fixture -------------------------------------------------------------------------------
$root = Join-Path ([IO.Path]::GetTempPath()) ("ExplorerMate.Tests\tabs-" + [guid]::NewGuid().ToString('N').Substring(0, 8))
$folderA = (New-Item -ItemType Directory -Force (Join-Path $root 'FolderA')).FullName
$folderB = (New-Item -ItemType Directory -Force (Join-Path $root 'FolderB')).FullName
foreach ($name in 'a1.txt', 'a2.txt', 'a3.txt') { Set-Content (Join-Path $folderA $name) 'x' }
foreach ($name in 'b1.txt', 'b2.txt') { Set-Content (Join-Path $folderB $name) 'x' }
$folderA = (Get-Item $folderA).FullName; $folderB = (Get-Item $folderB).FullName

$shell = New-Object -ComObject Shell.Application

function Get-FolderPath($window) { try { $window.Document.Folder.Self.Path } catch { $null } }
function Get-FrameTabs([long]$frame) { @($shell.Windows() | Where-Object { $_ -and $_.HWND -eq $frame }) }
function Wait-Until([scriptblock]$Condition, [string]$What, [int]$Seconds = 15) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $deadline) {
        $value = & $Condition
        if ($value) { return $value }
        Start-Sleep -Milliseconds 200
    }
    throw "Timed out waiting for: $What"
}
function Select-Items($window, [string[]]$Names) {
    $first = $true
    foreach ($name in $Names) {
        $item = $window.Document.Folder.ParseName($name)
        # 1 = select, 4 = deselect everything else, 8 = scroll into view.
        $flags = 1 + 8; if ($first) { $flags += 4 }
        $window.Document.SelectItem($item, $flags)
        $first = $false
    }
}

# --- Open a window on FolderA and find its frame -------------------------------------------
$before = @($shell.Windows() | ForEach-Object { $_.HWND })
Start-Process explorer.exe -ArgumentList "`"$folderA`""
$firstTab = Wait-Until { $shell.Windows() | Where-Object { $_ -and (Get-FolderPath $_) -eq $folderA } | Select-Object -First 1 } 'the new Explorer window'
$frame = [long]$firstTab.HWND
$openedNewFrame = $before -notcontains $frame
$automationRoot = [System.Windows.Automation.AutomationElement]::FromHandle([IntPtr]$frame)

function Find-ByAutomationId([string]$id) {
    $condition = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::AutomationIdProperty, $id)
    $automationRoot.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $condition)
}
function Get-TabItems {
    $condition = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::TabItem)
    @($automationRoot.FindAll([System.Windows.Automation.TreeScope]::Descendants, $condition))
}
function Add-Tab([string]$Path) {
    $count = @(Get-FrameTabs $frame).Count
    $addButton = Wait-Until { Find-ByAutomationId 'AddButton' } 'the new-tab button'
    $addButton.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    Wait-Until { @(Get-FrameTabs $frame).Count -eq $count + 1 } 'the new tab' | Out-Null
    # The new tab is the one that is not showing a fixture folder yet.
    $tab = Wait-Until { Get-FrameTabs $frame | Where-Object { (Get-FolderPath $_) -notin @($folderA, $folderB) } | Select-Object -First 1 } 'the new tab object'
    $tab.Navigate2($Path)
    Wait-Until { (Get-FolderPath $tab) -eq $Path } "navigation to $Path" | Out-Null
    return $tab
}

$failures = @()
try {
    $secondTab = Add-Tab $folderB
    $thirdTab = Add-Tab $folderA

    Start-Sleep -Milliseconds 500
    Select-Items $firstTab @('a1.txt')
    Select-Items $secondTab @('b1.txt', 'b2.txt')
    Select-Items $thirdTab @('a2.txt', 'a3.txt')

    $expected = @(
        'FolderA|a1.txt',
        'FolderB|b1.txt,b2.txt',
        'FolderA|a2.txt,a3.txt'
    )

    # --- Activate each tab and ask ExplorerMate what it sees ------------------------------
    $seen = @()
    $tabItems = @(Get-TabItems)
    if ($tabItems.Count -ne 3) { throw "Expected 3 tab items in the tab strip, found $($tabItems.Count)." }
    foreach ($tabItem in $tabItems) {
        $tabItem.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
        Start-Sleep -Milliseconds 700

        # The EXE is a GUI-subsystem program: PowerShell only waits for it and captures its
        # output when it is part of a pipeline.
        $lines = @(& $exe --diagnose-explorer | ForEach-Object { $_ })
        $frameHex = '0x{0:X8}' -f $frame
        $tabs = @()
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if ($lines[$i] -like "tab frame=$frameHex *") {
                $names = @()
                for ($j = $i + 4; $j -lt $lines.Count -and $lines[$j].StartsWith('    '); $j++) { $names += $lines[$j].Trim() }
                $tabs += [pscustomobject]@{
                    Active    = $lines[$i + 1] -match 'activeInFrame=yes'
                    Folder    = Split-Path ($lines[$i + 2] -replace '^\s*folder=', '') -Leaf
                    Selection = ($names | Sort-Object) -join ','
                }
            }
        }
        if ($tabs.Count -eq 0) {
            Write-Host "No tab matched frame $frameHex. Raw diagnostic output:"
            $lines | ForEach-Object { Write-Host "  | $_" }
        }
        $active = @($tabs | Where-Object Active)
        $observed = if ($active.Count -eq 1) { "$($active[0].Folder)|$($active[0].Selection)" } else { "<$($active.Count) active tabs>" }
        Write-Host ("UI tab '{0}': ExplorerMate sees {1} tabs in the frame, active -> {2}" -f $tabItem.Current.Name, $tabs.Count, $observed)
        if ($tabs.Count -ne 3) { $failures += "Expected 3 tabs in the frame, ExplorerMate saw $($tabs.Count)." }
        if ($active.Count -ne 1) { $failures += "Expected exactly one active tab, got $($active.Count)." }
        $seen += $observed
    }

    $missing = @($expected | Where-Object { $seen -notcontains $_ })
    if ($missing.Count -gt 0) { $failures += "Never detected as active: $($missing -join ' ; ')" }
    if (@($seen | Select-Object -Unique).Count -ne 3) { $failures += 'Two different UI tabs were reported as the same tab.' }

    # --- Focus gate: what would a keyboard shortcut act on right now? ----------------------
    # Needs the test window in the foreground. Windows may refuse that to a background
    # script; the checks are then skipped rather than failed.
    function Get-Diagnostics { @(& $exe --diagnose-explorer | ForEach-Object { $_ }) }
    $frameHex = '0x{0:X8}' -f $frame
    $listItemCondition = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::ListItem)
    $listItem = $automationRoot.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $listItemCondition)
    try { $listItem.SetFocus() } catch { }
    Start-Sleep -Milliseconds 600
    $diagnostics = Get-Diagnostics
    if ($diagnostics[0] -like "foreground=$frameHex *") {
        Write-Host "Focus in file list : $($diagnostics[0])"
        Write-Host "                     $($diagnostics[1])"
        if ($diagnostics[1] -notlike '*focusInFileList=yes target=`[a2.txt`]`[a3.txt`]*') {
            $failures += "With the file list focused, the shortcut target should be the active tab's selection."
        }

        # F2 opens the rename box of a file in the fixture; Escape leaves it without renaming.
        Add-Type -AssemblyName System.Windows.Forms
        if ((Get-Diagnostics)[0] -like "foreground=$frameHex *") {
            [System.Windows.Forms.SendKeys]::SendWait('{F2}')
            Start-Sleep -Milliseconds 700
            $renaming = Get-Diagnostics
            if ((Get-Diagnostics)[0] -like "foreground=$frameHex *") { [System.Windows.Forms.SendKeys]::SendWait('{ESC}') }
            Write-Host "Focus in rename box: $($renaming[0])"
            Write-Host "                     $($renaming[1])"
            if ($renaming[1] -notlike '*focusInFileList=no*') {
                $failures += 'While renaming, shortcuts must be refused.'
            }
        }
    } else {
        Write-Host "SKIPPED focus checks: the test window could not be brought to the foreground ($($diagnostics[0]))."
    }
}
finally {
    if (-not $KeepWindow -and $openedNewFrame) {
        [void][ET.Native]::PostMessageW([IntPtr]$frame, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)  # WM_CLOSE
        Start-Sleep -Milliseconds 800
    }
    if ($root -like '*\ExplorerMate.Tests\tabs-*') { Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue }
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Host "FAIL: $_" }
    throw 'Tab detection test failed.'
}
Write-Host 'PASS: every UI tab was detected as the single active tab with its own folder and selection.'
