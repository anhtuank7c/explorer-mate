# Registers this folder as the Explorer Mate package for the current user.
# FOR DEVELOPERS: the binaries are not signed, so this only works with Developer Mode on.
# Windows runs Explorer Mate from this folder afterwards: keep the folder where it is.
$ErrorActionPreference = 'Stop'

$manifest = Join-Path $PSScriptRoot 'AppxManifest.xml'
if (-not (Test-Path $manifest)) { throw "AppxManifest.xml is missing next to this script. Extract the whole zip first." }

$unlock = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock' -ErrorAction SilentlyContinue
if (-not $unlock -or $unlock.AllowDevelopmentWithoutDevLicense -ne 1) {
    Write-Host 'Developer Mode is off. Turn it on in Settings > System > For developers, then run this script again.'
    Write-Host 'If you are not a developer, install Explorer Mate from the Microsoft Store instead.'
    exit 1
}

$others = @(Get-AppxPackage | Where-Object { $_.Name -like '*ExplorerMate*' -and $_.InstallLocation -ne $PSScriptRoot })
if ($others.Count -gt 0) {
    Write-Host 'Another Explorer Mate package is already installed:'
    $others | ForEach-Object { Write-Host "  $($_.PackageFullName)" }
    Write-Host 'Uninstall it first; two copies would add the same menu commands twice.'
    exit 1
}

# Files extracted from a downloaded zip carry a "downloaded from the internet" mark.
Get-ChildItem $PSScriptRoot -Recurse -File | Unblock-File

Add-AppxPackage -Register $manifest
$package = Get-AppxPackage -Name 'ExplorerMate'
Write-Host "Registered $($package.PackageFullName)."
Write-Host 'Open "Explorer Mate" from the Start menu once to start the tray agent.'
Write-Host 'If Windows refuses to run it, Smart App Control is blocking the unsigned program; this build cannot be used on that computer.'
