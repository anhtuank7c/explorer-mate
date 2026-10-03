[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$NoBuild
)

. (Join-Path $PSScriptRoot 'Common.ps1')

& (Join-Path $PSScriptRoot 'check-layers.ps1')

if (-not $NoBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration
}

$outDir = Join-Path $RepoRoot "build\x64\$Configuration"
$testDlls = @('ExMate.UnitTests.dll', 'ExMate.IntegrationTests.dll') |
    ForEach-Object { Join-Path $outDir $_ }
foreach ($dll in $testDlls) {
    if (-not (Test-Path $dll)) { throw "Test binary not found: $dll" }
}

Write-Host "== Test $Configuration|x64 =="
& (Get-VsTestPath) $testDlls /Platform:x64
if ($LASTEXITCODE -ne 0) { throw "Tests failed (exit code $LASTEXITCODE)" }
