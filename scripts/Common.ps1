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

function Get-VsTestPath {
    $path = Join-Path (Get-VsInstallPath) 'Common7\IDE\Extensions\TestPlatform\vstest.console.exe'
    if (-not (Test-Path $path)) { throw "vstest.console.exe not found at $path" }
    return $path
}
