[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'All')]
    [string]$Configuration = 'Debug',
    # Embed the version resources (Task Manager then shows "Explorer Mate" instead of the file
    # name). Only for builds that will be signed: Smart App Control may block unsigned ones.
    [switch]$EmbedVersionInfo
)

. (Join-Path $PSScriptRoot 'Common.ps1')

$msbuild = Get-MSBuildPath
$solution = Join-Path $RepoRoot 'ExplorerMate.sln'
$configurations = if ($Configuration -eq 'All') { @('Debug', 'Release') } else { @($Configuration) }
$versionInfo = if ($EmbedVersionInfo) { 'true' } else { 'false' }

foreach ($config in $configurations) {
    Write-Host "== Build $config|x64 =="
    & $msbuild $solution /m /nologo /v:minimal "/p:Configuration=$config" /p:Platform=x64 "/p:EmbedVersionInfo=$versionInfo"
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $config|x64 (exit code $LASTEXITCODE)" }
}
