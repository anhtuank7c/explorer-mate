Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot

function Get-VsInstallPath {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { throw 'vswhere.exe not found. Install Visual Studio with the C++ workload.' }
    $path = & $vswhere -latest -prerelease -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $path) { throw 'No Visual Studio installation with the C++ toolset was found.' }
    return $path
}

function Get-MSBuildPath {
    $path = Join-Path (Get-VsInstallPath) 'MSBuild\Current\Bin\MSBuild.exe'
    if (-not (Test-Path $path)) { throw "MSBuild.exe not found at $path" }
    return $path
}

# The product version, read from its single source: src\Domain\Version.h.
function Get-ProductVersion {
    $header = Get-Content (Join-Path $RepoRoot 'src\Domain\Version.h') -Raw
    if ($header -notmatch '#define\s+ET_VERSION_STRING\s+"(\d+\.\d+\.\d+)"') {
        throw 'ET_VERSION_STRING not found in src\Domain\Version.h.'
    }
    return $Matches[1]
}

# Deletes a folder under build\ and everything in it. Refuses any other path, and refuses a
# tree containing links or junctions: a recursive delete would otherwise follow them and
# remove files outside the repository.
function Remove-BuildFolder([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    $buildRoot = [IO.Path]::GetFullPath((Join-Path $RepoRoot 'build')) + '\'
    if (-not $full.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase) -or $full.Length -le $buildRoot.Length) {
        throw "Refusing to delete '$full': it is not inside $buildRoot"
    }
    if (-not (Test-Path -LiteralPath $full)) { return }
    $links = @(Get-ChildItem -LiteralPath $full -Recurse -Force -Attributes ReparsePoint -ErrorAction SilentlyContinue)
    if ($links.Count -gt 0 -or ((Get-Item -LiteralPath $full -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Refusing to delete '$full': it contains a link or junction ($($links[0].FullName))."
    }
    [IO.Directory]::Delete($full, $true)
}

function Get-SdkTool([string]$Name) {
    $kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    $tool = Get-ChildItem $kits -Directory -Filter '10.*' | Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName "x64\$Name" } | Where-Object { Test-Path $_ } |
        Select-Object -First 1
    if (-not $tool) { throw "$Name not found in the Windows SDK." }
    return $tool
}

# Writes resources.pri into a package folder. Without it Windows only sees the plain logo
# files; with it, it picks the size-specific and "unplated" variants (Square44x44Logo
# .targetsize-16_altform-unplated.png and so on), so the taskbar and Start show a crisp icon
# without a coloured backplate. The folder must already contain AppxManifest.xml.
function New-PackageResourceIndex([string]$PackageFolder) {
    $makepri = Get-SdkTool 'makepri.exe'
    $workDir = Join-Path $RepoRoot 'build\pri'
    New-Item -ItemType Directory -Force $workDir | Out-Null
    $config = Join-Path $workDir 'priconfig.xml'
    & $makepri createconfig /cf $config /dq en-US /o | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "makepri createconfig failed (exit code $LASTEXITCODE)." }
    $index = Join-Path $PackageFolder 'resources.pri'
    & $makepri new /pr $PackageFolder /cf $config /mn (Join-Path $PackageFolder 'AppxManifest.xml') /of $index /o | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "makepri new failed (exit code $LASTEXITCODE)." }
}

function Get-VsTestPath {
    $path = Join-Path (Get-VsInstallPath) 'Common7\IDE\Extensions\TestPlatform\vstest.console.exe'
    if (-not (Test-Path $path)) { throw "vstest.console.exe not found at $path" }
    return $path
}
