# Builds the release MSIX: a full package with the binaries inside (not the sparse package
# used for development). Output: build\package\ExplorerMate_<version>_x64.msix.
#
# The package is unsigned unless -CertificateThumbprint is given. The Store signs packages
# itself; GitHub/WinGet downloads need your own certificate. The certificate is taken from
# the Windows certificate store by thumbprint, so no password or key file ever appears on a
# command line or in this repository.
#
#   -RegisterLoose   instead of packing, register an unpacked layout for the current user
#                    (needs Developer Mode). Lets the full package be tried without a
#                    certificate. Conflicts with the development package, which must be
#                    uninstalled first (scripts\uninstall-dev.ps1).
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    # Four-part package version. Defaults to the version in src\Domain\Version.h plus ".0".
    [ValidatePattern('^(\d+\.\d+\.\d+\.\d+)?$')][string]$Version = '',
    # Must match the Partner Center identity (Store) or the certificate subject (GitHub).
    [string]$IdentityName = 'ExplorerMate',
    [string]$Publisher = 'CN=ExplorerMate Dev',
    [string]$PublisherDisplayName = 'Tuan Nguyen',
    # SHA-1 thumbprint of a code-signing certificate in Cert:\CurrentUser\My.
    [ValidatePattern('^([0-9A-Fa-f]{40})?$')][string]$CertificateThumbprint = '',
    # RFC 3161 timestamp server, so the signature stays valid after the certificate expires.
    [string]$TimestampUrl = 'http://timestamp.digicert.com',
    [switch]$RegisterLoose
)

. (Join-Path $PSScriptRoot 'Common.ps1')

# Copies the binaries, logos, license texts and a filled-in manifest into $Directory.
function Write-PackageLayout([string]$Directory) {
    New-Item -ItemType Directory -Force $Directory | Out-Null
    foreach ($name in $binaries) {
        Copy-Item (Join-Path $binDir $name) (Join-Path $Directory $name) -Force
    }
    Copy-Item (Join-Path $RepoRoot 'packaging\Assets') $Directory -Recurse -Force
    # License texts travel with every copy of the binaries.
    Copy-Item (Join-Path $RepoRoot 'LICENSE') (Join-Path $Directory 'LICENSE.txt') -Force
    Copy-Item (Join-Path $RepoRoot 'THIRD_PARTY_NOTICES.md') (Join-Path $Directory 'THIRD_PARTY_NOTICES.md') -Force

    $manifest = [IO.File]::ReadAllText((Join-Path $RepoRoot 'packaging\release\AppxManifest.xml'))
    $tokens = @{
        '@@IDENTITY_NAME@@'          = $IdentityName
        '@@PUBLISHER@@'              = $Publisher
        '@@VERSION@@'                = $Version
        '@@PUBLISHER_DISPLAY_NAME@@' = $PublisherDisplayName
    }
    foreach ($token in $tokens.Keys) {
        $manifest = $manifest.Replace($token, [Security.SecurityElement]::Escape($tokens[$token]))
    }
    if ($manifest -match '@@') { throw 'The manifest still contains unreplaced tokens.' }
    [IO.File]::WriteAllText((Join-Path $Directory 'AppxManifest.xml'), $manifest, (New-Object Text.UTF8Encoding $false))
    New-PackageResourceIndex $Directory
}

if (-not $Version) { $Version = (Get-ProductVersion) + '.0' }
$binDir = Join-Path $RepoRoot "build\x64\$Configuration"
$packageDir = Join-Path $RepoRoot 'build\package'
$binaries = @('ExplorerMate.exe', 'ExplorerMate.Shell.dll')
foreach ($name in $binaries) {
    if (-not (Test-Path (Join-Path $binDir $name))) {
        throw "Missing $name in $binDir. Run scripts\build.ps1 -Configuration $Configuration first."
    }
}

# --- Loose registration (development only) -------------------------------------------------
if ($RegisterLoose) {
    if (Get-AppxPackage -Name 'ExplorerMate.Dev') {
        throw 'The development package "ExplorerMate.Dev" is registered. Both packages would add the same menu commands twice. Run scripts\uninstall-dev.ps1 first.'
    }
    # Updated in place: Windows runs the package from this folder, so it cannot be recreated.
    $layoutDir = Join-Path $packageDir 'layout'
    $layoutExe = Join-Path $layoutDir 'ExplorerMate.exe'
    $agentWasRunning = $false
    if (Test-Path $layoutExe) {
        # The old EXE may be refused by Smart App Control; then no agent can be running from it.
        try { $agentWasRunning = (& $layoutExe --stop-agent | Out-String) -match 'Agent stopped' } catch { }
    }
    Write-PackageLayout $layoutDir
    Add-AppxPackage -Register (Join-Path $layoutDir 'AppxManifest.xml')
    $package = Get-AppxPackage -Name $IdentityName
    Write-Host "Registered loose layout: $($package.PackageFullName)"
    if ($agentWasRunning) {
        Start-Process $layoutExe -ArgumentList '--agent'
        Write-Host 'Agent restarted.'
    }
    return
}

# --- Pack ----------------------------------------------------------------------------------
# Always from an empty staging folder, so nothing left over from an earlier run - or put
# there by something else - can end up inside a package that is about to be signed.
$stagingDir = Join-Path $packageDir 'staging'
Remove-BuildFolder $stagingDir
Write-PackageLayout $stagingDir

$msix = Join-Path $packageDir "ExplorerMate_${Version}_x64.msix"
& (Get-SdkTool 'makeappx.exe') pack /d $stagingDir /p $msix /o | Out-Null
if ($LASTEXITCODE -ne 0) { throw "makeappx failed (exit code $LASTEXITCODE)." }
Write-Host "Packed $msix"

# --- Sign (optional) -----------------------------------------------------------------------
if ($CertificateThumbprint) {
    & (Get-SdkTool 'signtool.exe') sign /fd SHA256 /sha1 $CertificateThumbprint /tr $TimestampUrl /td SHA256 $msix
    if ($LASTEXITCODE -ne 0) { throw "signtool failed (exit code $LASTEXITCODE)." }
    Write-Host 'Signed and timestamped.'
} else {
    Write-Host 'Not signed. The Store signs on submission; other channels need -CertificateThumbprint.'
}
Write-Host ("SHA256  {0}" -f (Get-FileHash $msix -Algorithm SHA256).Hash)
