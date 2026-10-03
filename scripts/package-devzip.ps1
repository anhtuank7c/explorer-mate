# Turns an unsigned MSIX into the "for developers" zip attached to GitHub releases: the
# package's files as a loose layout, plus Install.ps1 / Uninstall.ps1 / README.txt from
# packaging\devzip. The layout is registered with Developer Mode on; nothing in it is signed.
# Output: build\package\<msix name>_unsigned-dev.zip
[CmdletBinding()]
param(
    # The package to repack, e.g. the one from the Release workflow's artifact.
    [Parameter(Mandatory)][string]$Msix
)

. (Join-Path $PSScriptRoot 'Common.ps1')
Add-Type -AssemblyName System.IO.Compression.FileSystem

$msixFile = Get-Item -LiteralPath $Msix
$packageDir = Join-Path $RepoRoot 'build\package'
$stagingDir = Join-Path $packageDir 'devzip-staging'
Remove-BuildFolder $stagingDir
New-Item -ItemType Directory -Force $stagingDir | Out-Null

[IO.Compression.ZipFile]::ExtractToDirectory($msixFile.FullName, $stagingDir)
# Packaging metadata that only means something inside a packed .msix.
foreach ($name in 'AppxBlockMap.xml', '[Content_Types].xml', 'AppxSignature.p7x') {
    $path = Join-Path $stagingDir $name
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
}
foreach ($required in 'AppxManifest.xml', 'ExplorerMate.exe', 'ExplorerMate.Shell.dll') {
    if (-not (Test-Path (Join-Path $stagingDir $required))) { throw "$required is missing from $($msixFile.Name)." }
}
Copy-Item (Join-Path $RepoRoot 'packaging\devzip\*') $stagingDir -Force

$hash = (Get-FileHash $msixFile.FullName -Algorithm SHA256).Hash.ToLower()
@(
    "Source package: $($msixFile.Name)"
    "SHA-256:        $hash"
    "ExplorerMate.exe SHA-256:       $((Get-FileHash (Join-Path $stagingDir 'ExplorerMate.exe') -Algorithm SHA256).Hash.ToLower())"
    "ExplorerMate.Shell.dll SHA-256: $((Get-FileHash (Join-Path $stagingDir 'ExplorerMate.Shell.dll') -Algorithm SHA256).Hash.ToLower())"
) | Set-Content (Join-Path $stagingDir 'BUILD-INFO.txt') -Encoding ascii

$zip = Join-Path $packageDir ($msixFile.BaseName + '_unsigned-dev.zip')
if (Test-Path $zip) { Remove-Item $zip -Force }
# Entry names are written with forward slashes by hand: Windows PowerShell's own zip helpers
# write backslashes, which other unzip tools treat as part of the file name.
$archive = [IO.Compression.ZipFile]::Open($zip, [IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($file in Get-ChildItem $stagingDir -Recurse -File) {
        $entryName = $file.FullName.Substring($stagingDir.Length + 1).Replace('\', '/')
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $entryName)
    }
} finally {
    $archive.Dispose()
}
Write-Host "Wrote $zip"
Write-Host ("SHA256  {0}" -f (Get-FileHash $zip -Algorithm SHA256).Hash)
