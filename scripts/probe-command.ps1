# Activates a registered ExplorerMate command through COM, the way File Explorer does, and
# calls Invoke on the given items. Lets the packaged activation path be tested without the GUI.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][guid]$Clsid,
    [Parameter(Mandatory)][string[]]$Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

[ComImport, Guid("a08ce4d0-fa25-44ab-b57c-c7b1c323e0b9"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IExplorerCommandProbe {
    [PreserveSig] int GetTitle(IntPtr items, out IntPtr name);
    [PreserveSig] int GetIcon(IntPtr items, out IntPtr icon);
    [PreserveSig] int GetToolTip(IntPtr items, out IntPtr tip);
    [PreserveSig] int GetCanonicalName(out Guid name);
    [PreserveSig] int GetState(IntPtr items, int okToBeSlow, out int state);
    [PreserveSig] int Invoke(IntPtr items, IntPtr bindContext);
    [PreserveSig] int GetFlags(out int flags);
    [PreserveSig] int EnumSubCommands(out IntPtr commands);
}

public static class ExplorerCommandProbe {
    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    static extern int SHParseDisplayName(string name, IntPtr bindContext, out IntPtr pidl, uint sfgaoIn, out uint sfgaoOut);
    [DllImport("shell32.dll")]
    static extern int SHCreateShellItemArrayFromIDLists(uint count, IntPtr[] pidls, out IntPtr array);
    [DllImport("ole32.dll")]
    static extern void CoTaskMemFree(IntPtr memory);

    public static string Run(Guid clsid, string[] paths) {
        IntPtr[] pidls = new IntPtr[paths.Length];
        try {
            for (int i = 0; i < paths.Length; i++) {
                uint attributes;
                Marshal.ThrowExceptionForHR(SHParseDisplayName(paths[i], IntPtr.Zero, out pidls[i], 0, out attributes));
            }
            IntPtr array;
            Marshal.ThrowExceptionForHR(SHCreateShellItemArrayFromIDLists((uint)pidls.Length, pidls, out array));
            try {
                IExplorerCommandProbe command = (IExplorerCommandProbe)Activator.CreateInstance(Type.GetTypeFromCLSID(clsid));
                IntPtr title;
                int titleResult = command.GetTitle(array, out title);
                int state;
                int stateResult = command.GetState(array, 0, out state);
                int invokeResult = command.Invoke(array, IntPtr.Zero);
                string text = Marshal.PtrToStringUni(title);
                CoTaskMemFree(title);
                Marshal.ReleaseComObject(command);
                return string.Format("GetTitle=0x{0:X8} '{1}'; GetState=0x{2:X8} state={3}; Invoke=0x{4:X8}",
                    titleResult, text, stateResult, state, invokeResult);
            } finally {
                Marshal.Release(array);
            }
        } finally {
            foreach (IntPtr pidl in pidls) { if (pidl != IntPtr.Zero) CoTaskMemFree(pidl); }
        }
    }
}
'@

$resolved = $Path | ForEach-Object { (Resolve-Path -LiteralPath $_).ProviderPath }
[ExplorerCommandProbe]::Run($Clsid, [string[]]$resolved)
