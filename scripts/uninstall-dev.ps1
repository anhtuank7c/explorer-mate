# Removes the development sparse package for the current user and its staged binaries.
[CmdletBinding()]
param()

. (Join-Path $PSScriptRoot 'Common.ps1')

$PackageName = 'ExMate.Dev'
$stageDir = Join-Path $RepoRoot 'build\install'

# Stop the agent and drop its "Start with Windows" entry, but only if that entry points at
# this development install.
$stagedExe = Join-Path $stageDir 'ExMate.exe'
if (Test-Path $stagedExe) {
    & $stagedExe --stop-agent | Out-Null
}
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$runValue = (Get-ItemProperty $runKey -ErrorAction SilentlyContinue).PSObject.Properties |
    Where-Object { $_.Name -eq 'ExMate' }
if ($runValue -and ([string]$runValue.Value).IndexOf($stageDir, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
    Remove-ItemProperty $runKey -Name 'ExMate'
    Write-Host 'Removed the "Start with Windows" entry.'
}

$packages = @(Get-AppxPackage -Name $PackageName)
if ($packages.Count -eq 0) {
    Write-Host "$PackageName is not registered."
}
foreach ($package in $packages) {
    Remove-AppxPackage -Package $package.PackageFullName
    Write-Host "Removed $($package.PackageFullName)"
}

if (Test-Path (Join-Path $stageDir 'AppxManifest.xml')) {
    try {
        Remove-Item $stageDir -Recurse -Force
        Write-Host "Deleted $stageDir"
    } catch {
        Write-Host "Could not delete $stageDir (a file is still loaded). It is safe to delete after File Explorer restarts."
    }
}
