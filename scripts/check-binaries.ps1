# Scans the built EXE and DLL with Microsoft BinSkim for missing exploit mitigations
# (Control Flow Guard, CET, ASLR, DEP, stack protection, DLL search path, ...).
# Fails on BinSkim errors. Warnings are printed; two are expected and accepted:
#   BA2024  Spectre mitigations: needs the optional "spectre-mitigated libs" component and
#           is aimed at code that keeps secrets from other code in the same process.
#   BA2027  SourceLink information in the PDB.
#
# BinSkim is downloaded from nuget.org into build\tools on first use.
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [string]$BinSkimVersion = '4.4.9.11'
)

. (Join-Path $PSScriptRoot 'Common.ps1')

$toolDir = Join-Path $RepoRoot "build\tools\binskim-$BinSkimVersion"
$binskim = Join-Path $toolDir 'tools\net9.0\win-x64\BinSkim.exe'
if (-not (Test-Path $binskim)) {
    New-Item -ItemType Directory -Force $toolDir | Out-Null
    $package = Join-Path $toolDir 'binskim.zip'
    $previous = $ProgressPreference; $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest "https://www.nuget.org/api/v2/package/Microsoft.CodeAnalysis.BinSkim/$BinSkimVersion" -OutFile $package -UseBasicParsing
    $ProgressPreference = $previous
    Expand-Archive $package -DestinationPath $toolDir -Force
    Remove-Item $package
    if (-not (Test-Path $binskim)) { throw "BinSkim.exe not found in the downloaded package ($toolDir)." }
}

$binDir = Join-Path $RepoRoot "build\x64\$Configuration"
$targets = @('ExplorerMate.exe', 'ExplorerMate.Shell.dll') | ForEach-Object { Join-Path $binDir $_ }
foreach ($target in $targets) {
    if (-not (Test-Path $target)) { throw "Missing $target. Build the $Configuration configuration first." }
}

$sarif = Join-Path $RepoRoot "build\binskim-$Configuration.sarif"
if (Test-Path $sarif) { Remove-Item $sarif }
& $binskim analyze @targets --output $sarif --disable-telemetry | Out-Null
if (-not (Test-Path $sarif)) { throw "BinSkim produced no report (exit code $LASTEXITCODE)." }

# SARIF omits "level" when it is the default, which is "warning".
function Get-Level($result) {
    if ($result.PSObject.Properties.Name -contains 'level') { return $result.level }
    return 'warning'
}
$results = @((Get-Content $sarif -Raw | ConvertFrom-Json).runs[0].results)
$errors = @($results | Where-Object { (Get-Level $_) -eq 'error' })
$warnings = @($results | Where-Object { (Get-Level $_) -eq 'warning' })
foreach ($result in ($errors + $warnings)) {
    $file = Split-Path $result.locations[0].physicalLocation.artifactLocation.uri -Leaf
    Write-Host ("{0} {1} {2}" -f (Get-Level $result).ToUpper(), $result.ruleId, $file)
}
Write-Host "BinSkim: $($errors.Count) error(s), $($warnings.Count) warning(s). Report: $sarif"
if ($errors.Count -gt 0) { throw 'BinSkim reported errors.' }
