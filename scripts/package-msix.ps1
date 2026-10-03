# Builds the release MSIX: a full package with the binaries inside (not the sparse package
# used for development). Output: build\package\ExMate_<version>_x64.msix (unsigned unless a
# certificate is given). The Store signs packages itself; for GitHub/WinGet pass -CertificatePath.
#
#   -RegisterLoose   instead of using the .msix, register the unpacked layout for the current
#                    user (needs Developer Mode). Lets the full package be tried without a
#                    certificate. Conflicts with the development package, which must be
#                    uninstalled first (scripts\uninstall-dev.ps1).
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [ValidatePattern('^\d+\.\d+\.\d+\.\d+$')][string]$Version = '0.1.0.0',
    # Must match the Partner Center identity (Store) or the certificate subject (GitHub).
    [string]$IdentityName = 'ExMate',
    [string]$Publisher = 'CN=ExMate Dev',
    [string]$PublisherDisplayName = 'ExMate Dev',
    [string]$CertificatePath,
    [securestring]$CertificatePassword,
    [switch]$RegisterLoose
)

. (Join-Path $PSScriptRoot 'Common.ps1')

function Get-SdkTool([string]$name) {
    $kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    $tool = Get-ChildItem $kits -Directory -Filter '10.*' | Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName "x64\$name" } | Where-Object { Test-Path $_ } |
        Select-Object -First 1
    if (-not $tool) { throw "$name not found in the Windows SDK." }
    return $tool
}

$binDir = Join-Path $RepoRoot "build\x64\$Configuration"
$packageDir = Join-Path $RepoRoot 'build\package'
$layoutDir = Join-Path $packageDir 'layout'
$binaries = @('ExMate.exe', 'ExMate.Shell.dll')
foreach ($name in $binaries) {
    if (-not (Test-Path (Join-Path $binDir $name))) {
        throw "Missing $name in $binDir. Run scripts\build.ps1 -Configuration $Configuration first."
    }
}

if ($RegisterLoose -and (Get-AppxPackage -Name 'ExMate.Dev')) {
    throw 'The development package "ExMate.Dev" is registered. Both packages would add the same menu commands twice. Run scripts\uninstall-dev.ps1 first.'
}

# A previously registered loose layout may have its agent running from the layout folder.
$layoutExe = Join-Path $layoutDir 'ExMate.exe'
$agentWasRunning = $false
if (Test-Path $layoutExe) {
    $agentWasRunning = (& $layoutExe --stop-agent | Out-String) -match 'Agent stopped'
}

# --- Layout ------------------------------------------------------------------------------
New-Item -ItemType Directory -Force $layoutDir | Out-Null
foreach ($name in $binaries) {
    Copy-Item (Join-Path $binDir $name) (Join-Path $layoutDir $name) -Force
}
Copy-Item (Join-Path $RepoRoot 'packaging\Assets') $layoutDir -Recurse -Force

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
[IO.File]::WriteAllText((Join-Path $layoutDir 'AppxManifest.xml'), $manifest, (New-Object Text.UTF8Encoding $false))

if ($RegisterLoose) {
    Add-AppxPackage -Register (Join-Path $layoutDir 'AppxManifest.xml')
    $package = Get-AppxPackage -Name $IdentityName
    Write-Host "Registered loose layout: $($package.PackageFullName)"
    if ($agentWasRunning) {
        Start-Process $layoutExe -ArgumentList '--agent'
        Write-Host 'Agent restarted.'
    }
    return
}

# --- Pack --------------------------------------------------------------------------------
$msix = Join-Path $packageDir "ExMate_${Version}_x64.msix"
& (Get-SdkTool 'makeappx.exe') pack /d $layoutDir /p $msix /o | Out-Null
if ($LASTEXITCODE -ne 0) { throw "makeappx failed (exit code $LASTEXITCODE)." }
Write-Host "Packed $msix"

# --- Sign (optional) ---------------------------------------------------------------------
if ($CertificatePath) {
    $arguments = @('sign', '/fd', 'SHA256', '/f', $CertificatePath)
    if ($CertificatePassword) {
        $plain = [Runtime.InteropServices.Marshal]::PtrToStringBSTR(
            [Runtime.InteropServices.Marshal]::SecureStringToBSTR($CertificatePassword))
        $arguments += @('/p', $plain)
    }
    & (Get-SdkTool 'signtool.exe') @arguments $msix
    if ($LASTEXITCODE -ne 0) { throw "signtool failed (exit code $LASTEXITCODE)." }
    Write-Host 'Signed.'
} else {
    Write-Host 'Not signed. The Store signs on submission; other channels need -CertificatePath.'
}
