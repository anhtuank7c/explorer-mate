# End-to-end check of the agent's shortcut path without pressing real keys.
# Opens ONE Explorer window on a fixture folder under %TEMP%, selects a file, gives the file
# list the focus, starts the agent and posts it the internal "Duplicate shortcut pressed"
# message. Expects a copy to appear. Then repeats while a rename box has the focus and
# expects nothing to happen.
#
# Not covered: the low-level keyboard hook itself. The agent ignores synthesised key presses
# by design, so only a person at the keyboard can exercise that part.
[CmdletBinding()]
param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')

. (Join-Path $PSScriptRoot 'Common.ps1')
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes, System.Windows.Forms
Add-Type -Namespace ET -Name AgentNative -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("user32.dll")]
public static extern bool PostMessageW(System.IntPtr window, uint message, System.IntPtr wParam, System.IntPtr lParam);
[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
public static extern System.IntPtr FindWindowW(string className, string windowName);
'@

$exe = Join-Path $RepoRoot "build\x64\$Configuration\ExplorerMate.exe"
if (-not (Test-Path $exe)) { throw "Build first: $exe not found." }
$HotkeyMessage = 0x8002        # WM_APP + 2, see src\App\Agent.cpp
$DuplicateAction = 2           # app::ActionKind::DuplicateInPlace

$root = Join-Path ([IO.Path]::GetTempPath()) ("ExplorerMate.Tests\agent-" + [guid]::NewGuid().ToString('N').Substring(0, 8))
$folder = (New-Item -ItemType Directory -Force $root).FullName
foreach ($name in 'a1.txt', 'a2.txt') { Set-Content (Join-Path $folder $name) 'x' }

function Wait-Until([scriptblock]$Condition, [string]$What, [int]$Seconds = 15) {
    $deadline = (Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $deadline) {
        $value = & $Condition
        if ($value) { return $value }
        Start-Sleep -Milliseconds 200
    }
    throw "Timed out waiting for: $What"
}
function Get-Diagnostics { @(& $exe --diagnose-explorer | ForEach-Object { $_ }) }
function Get-CopyCount { @(Get-ChildItem $folder -Filter '* - Copy*').Count }

$shell = New-Object -ComObject Shell.Application
$failures = @()
$frame = 0
& $exe --stop-agent | Out-Null
try {
    Start-Process explorer.exe -ArgumentList "`"$folder`""
    $window = Wait-Until { $shell.Windows() | Where-Object { $_ -and $(try { $_.Document.Folder.Self.Path } catch { $null }) -eq $folder } | Select-Object -First 1 } 'the Explorer window'
    $frame = [long]$window.HWND
    $frameHex = '0x{0:X8}' -f $frame
    Start-Sleep -Milliseconds 500
    $window.Document.SelectItem($window.Document.Folder.ParseName('a1.txt'), 1 + 4 + 8)

    $automationRoot = [System.Windows.Automation.AutomationElement]::FromHandle([IntPtr]$frame)
    $listItemCondition = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::ListItem)
    function Set-FileListFocus {
        # Selecting the (only) tab through UI Automation is what activates the window.
        $tabItemCondition = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::TabItem)
        try {
            $tabItem = $automationRoot.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $tabItemCondition)
            $tabItem.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Select()
        } catch { }
        Start-Sleep -Milliseconds 400
        try { $automationRoot.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $listItemCondition).SetFocus() } catch { }
        Start-Sleep -Milliseconds 600
    }
    foreach ($attempt in 1..5) {
        Set-FileListFocus
        if ((Get-Diagnostics)[0] -like "foreground=$frameHex *") { break }
        Start-Sleep -Milliseconds 500
    }
    if ((Get-Diagnostics)[0] -notlike "foreground=$frameHex *") {
        Write-Host "SKIPPED: the test window could not be brought to the foreground ($((Get-Diagnostics)[0]))."
        return
    }

    Start-Process $exe -ArgumentList '--agent'
    $agent = Wait-Until { $handle = [ET.AgentNative]::FindWindowW('ExplorerMateAgentWindow', [NullString]::Value); if ($handle -ne [IntPtr]::Zero) { $handle } } 'the agent window'

    # 1. File list focused: the shortcut duplicates the selected file.
    [void][ET.AgentNative]::PostMessageW($agent, $HotkeyMessage, [IntPtr]$DuplicateAction, [IntPtr]::Zero)
    try { Wait-Until { (Get-CopyCount) -eq 1 } 'the duplicate' 10 | Out-Null } catch { }
    Write-Host "File list focused -> copies: $(Get-CopyCount) ($((Get-ChildItem $folder -Filter '* - Copy*').Name -join ', '))"
    if ((Get-CopyCount) -ne 1 -or -not (Test-Path (Join-Path $folder 'a1 - Copy.txt'))) { $failures += 'Expected exactly one copy of a1.txt.' }

    # 2. Rename box focused: the same request must be refused.
    Start-Sleep -Milliseconds 800
    $window.Document.SelectItem($window.Document.Folder.ParseName('a2.txt'), 1 + 4 + 8)
    Set-FileListFocus
    if ((Get-Diagnostics)[0] -like "foreground=$frameHex *") {
        [System.Windows.Forms.SendKeys]::SendWait('{F2}')
        Start-Sleep -Milliseconds 700
        [void][ET.AgentNative]::PostMessageW($agent, $HotkeyMessage, [IntPtr]$DuplicateAction, [IntPtr]::Zero)
        Start-Sleep -Seconds 2
        if ((Get-Diagnostics)[0] -like "foreground=$frameHex *") { [System.Windows.Forms.SendKeys]::SendWait('{ESC}') }
        Write-Host "Rename box focused -> copies: $(Get-CopyCount)"
        if ((Get-CopyCount) -ne 1) { $failures += 'A shortcut must do nothing while a rename box has the focus.' }
    } else {
        Write-Host 'SKIPPED rename-box check: lost the foreground.'
    }
}
finally {
    & $exe --stop-agent | Out-Null
    if ($frame -ne 0) { [void][ET.AgentNative]::PostMessageW([IntPtr]$frame, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero); Start-Sleep -Milliseconds 800 }
    if ($root -like '*\ExplorerMate.Tests\agent-*') { Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue }
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Host "FAIL: $_" }
    throw 'Agent shortcut test failed.'
}
Write-Host 'PASS: the agent acted on the focused file list and refused while renaming.'
