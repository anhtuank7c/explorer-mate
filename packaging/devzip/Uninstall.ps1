# Removes the Explorer Mate package registered by Install.ps1. The folder can be deleted afterwards.
$ErrorActionPreference = 'Stop'

$package = Get-AppxPackage -Name 'ExplorerMate'
if (-not $package) {
    Write-Host 'Explorer Mate (developer build) is not registered.'
    exit 0
}
$exe = Join-Path $package.InstallLocation 'ExplorerMate.exe'
# Ask the tray agent to exit; a blocked or missing program is not a reason to stop here.
if (Test-Path $exe) { try { & $exe --stop-agent | Out-Null } catch { } }
Remove-AppxPackage -Package $package.PackageFullName
Write-Host "Removed $($package.PackageFullName)."
