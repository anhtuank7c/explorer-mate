# Starts ExplorerMate.exe interactively and drives its dialog from outside the process:
# reads the controls, optionally types into them, then presses OK or Cancel.
# Lets the dialogs be smoke-tested without a person at the keyboard.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('group', 'rename')][string]$Action,
    [Parameter(Mandatory)][string[]]$Path,
    [hashtable]$SetText = @{},          # control id -> text to type before reading the state
    [int[]]$Click = @(),                # button ids to click after typing (see src\App\resource.h)
    [ValidateSet('ok', 'cancel')][string]$Press = 'cancel',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release'
)

. (Join-Path $PSScriptRoot 'Common.ps1')

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class DialogProbe {
    public delegate bool EnumProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc callback, IntPtr parameter);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr dialog, int id);
    [DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr window);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern IntPtr SendMessageW(IntPtr window, uint message, IntPtr wParam, StringBuilder lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern IntPtr SendMessageW(IntPtr window, uint message, IntPtr wParam, string lParam);
    [DllImport("user32.dll")] public static extern IntPtr SendMessageW(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);

    public static IntPtr FindDialog(uint processId) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((window, parameter) => {
            uint owner;
            GetWindowThreadProcessId(window, out owner);
            if (owner == processId && IsWindowVisible(window)) { found = window; return false; }
            return true;
        }, IntPtr.Zero);
        return found;
    }
    public static string GetText(IntPtr window) {
        StringBuilder text = new StringBuilder(1024);
        SendMessageW(window, 0x000D /* WM_GETTEXT */, (IntPtr)text.Capacity, text);
        return text.ToString();
    }
    // Typing through WM_SETTEXT makes the edit control send EN_CHANGE like real input does.
    public static void SetText(IntPtr window, string text) { SendMessageW(window, 0x000C /* WM_SETTEXT */, IntPtr.Zero, text); }
}
'@

$exe = Join-Path $RepoRoot "build\x64\$Configuration\ExplorerMate.exe"
$arguments = @('--action', $Action) + ($Path | ForEach-Object { '"' + $_ + '"' })
$process = Start-Process $exe -ArgumentList $arguments -PassThru

$dialog = [IntPtr]::Zero
foreach ($attempt in 1..50) {
    Start-Sleep -Milliseconds 100
    $dialog = [DialogProbe]::FindDialog($process.Id)
    if ($dialog -ne [IntPtr]::Zero) { break }
}
if ($dialog -eq [IntPtr]::Zero) {
    if (-not $process.HasExited) { $process.Kill() }
    throw 'The dialog did not appear.'
}

foreach ($id in $SetText.Keys) {
    [DialogProbe]::SetText([DialogProbe]::GetDlgItem($dialog, [int]$id), [string]$SetText[$id])
}
foreach ($id in $Click) {
    [void][DialogProbe]::SendMessageW($dialog, 0x0111, [IntPtr]$id, [IntPtr]::Zero)
}
Start-Sleep -Milliseconds 200

$ids = @{ Name = 1001; Error = 1002; NameMask = 1003; ExtensionMask = 1004; Start = 1005; Preview = 1006
          Step = 1007; Digits = 1008; Search = 1009; Replace = 1010 }
$state = [ordered]@{
    Title      = [DialogProbe]::GetText($dialog)
    Foreground = ([DialogProbe]::GetForegroundWindow() -eq $dialog)
    Error      = [DialogProbe]::GetText([DialogProbe]::GetDlgItem($dialog, $ids.Error))
    OkEnabled  = [DialogProbe]::IsWindowEnabled([DialogProbe]::GetDlgItem($dialog, 1))
}
if ($Action -eq 'group') {
    $state.Name = [DialogProbe]::GetText([DialogProbe]::GetDlgItem($dialog, $ids.Name))
} else {
    foreach ($field in 'NameMask', 'ExtensionMask', 'Start', 'Step', 'Digits', 'Search', 'Replace') {
        $state[$field] = [DialogProbe]::GetText([DialogProbe]::GetDlgItem($dialog, $ids[$field]))
    }
    $state.PreviewRows = [int][DialogProbe]::SendMessageW([DialogProbe]::GetDlgItem($dialog, $ids.Preview), 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
}

$button = if ($Press -eq 'ok') { 1 } else { 2 }
[void][DialogProbe]::PostMessageW($dialog, 0x0111, [IntPtr]$button, [IntPtr]::Zero)
if (-not $process.WaitForExit(15000)) {
    $process.Kill()
    throw 'The worker did not exit after the dialog was closed.'
}
$state.ExitCode = $process.ExitCode
[pscustomobject]$state
