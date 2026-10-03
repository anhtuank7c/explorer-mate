# Registers the development sparse package for the current user. Requires Developer Mode
# because the package is not signed. Only touches the ExplorerMate.Dev package identity.
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    # Also start the background agent that provides the keyboard shortcuts.
    [switch]$StartAgent
)

. (Join-Path $PSScriptRoot 'Common.ps1')

$PackageName = 'ExplorerMate.Dev'
$binDir = Join-Path $RepoRoot "build\x64\$Configuration"
$stageDir = Join-Path $RepoRoot 'build\install'
$binaries = @('ExplorerMate.exe', 'ExplorerMate.Shell.dll')

function Test-DeveloperMode {
    $key = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock' -ErrorAction SilentlyContinue
    return ($null -ne $key) -and
        ($key.PSObject.Properties.Name -contains 'AllowDevelopmentWithoutDevLicense') -and
        ($key.AllowDevelopmentWithoutDevLicense -eq 1)
}

if (Get-AppxPackage -Name 'ExplorerMate') {
    throw 'The release-style package "ExplorerMate" is registered (scripts\package-msix.ps1 -RegisterLoose). Both packages would add the same menu commands twice. Remove it first: Get-AppxPackage -Name ExplorerMate | Remove-AppxPackage'
}

if (-not (Test-DeveloperMode)) {
    throw 'Developer Mode is off. Turn it on in Settings > System > For developers, then run this script again.'
}

foreach ($name in $binaries) {
    if (-not (Test-Path (Join-Path $binDir $name))) {
        throw "Missing $name in $binDir. Run scripts\build.ps1 -Configuration $Configuration first."
    }
}

# A running agent keeps the staged EXE locked; ask it to exit and remember to bring it back.
# This must come before the package is removed: removal closes the agent by itself, and it
# would then look as if no agent had been running.
$stagedExe = Join-Path $stageDir 'ExplorerMate.exe'
$agentWasRunning = $false
if (Test-Path $stagedExe) {
    # The old EXE may be refused by Smart App Control; then no agent can be running from it.
    try { $agentWasRunning = (& $stagedExe --stop-agent | Out-String) -match 'Agent stopped' } catch { }
}

Get-AppxPackage -Name $PackageName | ForEach-Object {
    Write-Host "Removing existing registration $($_.PackageFullName)"
    Remove-AppxPackage -Package $_.PackageFullName
}

# Binaries are staged outside the build output so a loaded DLL never blocks the next build.
New-Item -ItemType Directory -Force $stageDir | Out-Null
try {
    foreach ($name in $binaries) {
        Copy-Item (Join-Path $binDir $name) (Join-Path $stageDir $name) -Force
    }
} catch {
    throw "Cannot replace files in $stageDir ($($_.Exception.Message)). The shell extension is probably still loaded: restart File Explorer yourself (Task Manager > Windows Explorer > Restart), then run this script again."
}
# The manifest's version follows the single source in src\Domain\Version.h.
$manifest = [IO.File]::ReadAllText((Join-Path $RepoRoot 'packaging\AppxManifest.xml'))
$versioned = [regex]::Replace($manifest, '(<Identity\b[^>]*?\bVersion=")[^"]*(")', "`${1}$(Get-ProductVersion).0`${2}")
if ($versioned -notmatch ('Version="' + [regex]::Escape((Get-ProductVersion)) + '\.0"')) { throw 'Could not set the package version in the manifest.' }
[IO.File]::WriteAllText((Join-Path $stageDir 'AppxManifest.xml'), $versioned, (New-Object Text.UTF8Encoding $false))
Copy-Item (Join-Path $RepoRoot 'packaging\Assets') $stageDir -Recurse -Force

Add-AppxPackage -Register (Join-Path $stageDir 'AppxManifest.xml') -ExternalLocation $stageDir

$package = Get-AppxPackage -Name $PackageName
if (-not $package) { throw 'Registration reported success but the package is not listed.' }
Write-Host "Registered $($package.PackageFullName)"
Write-Host "External location: $stageDir"

if ($StartAgent -or $agentWasRunning) {
    Start-Process $stagedExe -ArgumentList '--agent'
    Write-Host 'Agent started (tray icon "Explorer Mate").'
}
