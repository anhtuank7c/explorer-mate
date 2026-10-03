[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'All')]
    [string]$Configuration = 'Debug',
    # Embed the version resources (Task Manager then shows "Explorer Mate" instead of the file
    # name). Only for builds that will be signed: Smart App Control may block unsigned ones.
    [switch]$EmbedVersionInfo,
    # Run MSVC static code analysis (/analyze) and fail the build on any finding.
    [switch]$Analyze
)

. (Join-Path $PSScriptRoot 'Common.ps1')

$msbuild = Get-MSBuildPath
$solution = Join-Path $RepoRoot 'ExplorerMate.sln'
$configurations = if ($Configuration -eq 'All') { @('Debug', 'Release') } else { @($Configuration) }
$versionInfo = if ($EmbedVersionInfo) { 'true' } else { 'false' }

$arguments = @('/m', '/nologo', '/v:minimal', '/p:Platform=x64', "/p:EmbedVersionInfo=$versionInfo")
if ($Analyze) {
    # Rebuild so every file is analysed, not only the ones that changed.
    $arguments += @('/t:Rebuild', '/p:RunCodeAnalysis=true', '/p:EnableMicrosoftCodeAnalysis=true',
        '/p:CodeAnalysisRuleSet=NativeRecommendedRules.ruleset', '/p:CodeAnalysisTreatWarningsAsErrors=true')
}

foreach ($config in $configurations) {
    Write-Host "== Build $config|x64 =="
    & $msbuild $solution @arguments "/p:Configuration=$config"
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $config|x64 (exit code $LASTEXITCODE)" }
}
