# Enforces the dependency rule: inner layers must not include outer layers or platform headers.
. (Join-Path $PSScriptRoot 'Common.ps1')

$platformHeaders = '<(windows|objbase|unknwn|shobjidl|shobjidl_core|shlobj|shlwapi|shellapi|combaseapi|wrl[/\\][^>]*|atl[^>]*)\.h>'
$rules = @(
    @{ Layer = 'Domain';         Forbidden = @('"Application/', '"Infrastructure/', '"App/', '"ShellExtension/', $platformHeaders) },
    @{ Layer = 'Application';    Forbidden = @('"Infrastructure/', '"App/', '"ShellExtension/', $platformHeaders) },
    @{ Layer = 'Infrastructure'; Forbidden = @('"App/', '"ShellExtension/') }
)

$violations = @()
foreach ($rule in $rules) {
    $files = Get-ChildItem (Join-Path $RepoRoot "src\$($rule.Layer)") -Recurse -Include *.h, *.cpp
    foreach ($pattern in $rule.Forbidden) {
        $violations += @($files | Select-String -Pattern "^\s*#\s*include\s+.*$pattern" |
            ForEach-Object { "$($_.Path):$($_.LineNumber): $($_.Line.Trim())" })
    }
}

if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Host $_ }
    throw "Layer check failed: $($violations.Count) forbidden include(s)."
}
Write-Host 'Layer check passed.'
