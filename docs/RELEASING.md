# Releasing Explorer Mate

What is automated, what a person must do, and what is not set up yet.

## Channels

| Channel | Who signs | Status |
|---|---|---|
| Microsoft Store | Microsoft, on submission | Product `9PB48F4K2G29`: version 0.1.0 passed certification on 4 October 2026 and is published. Its identity is in `packaging/store/identity.json` and `package-msix.ps1 -Store` builds the package to upload. |
| WinGet | — | Available through the `msstore` source once the Store listing exists. A manifest in `winget-pkgs` needs the signed GitHub download. |
| GitHub download | The maintainer's certificate | **No certificate yet.** A Store-signed package cannot be redistributed, so this channel needs its own trusted code-signing certificate; the application to SignPath Foundation is drafted in `docs/SIGNPATH.md`. Until then releases carry an unsigned zip for developers (below). |

Until a package is signed by a certificate Windows trusts, it can only be registered with Developer Mode on, and Smart App Control may refuse to run the binaries.

## Release checklist

1. **Version.** Set the new version in `src/Domain/Version.h` (the only place). Move the "Unreleased" entries in `CHANGELOG.md` under the new version and date.
2. **Verify locally.**
   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration All
   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test.ps1 -Configuration Release -NoBuild
   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration Debug -Analyze
   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\check-binaries.ps1
   ```
   Then go through the manual rows of `docs/TEST_MATRIX.md` in File Explorer (menu, dialogs, shortcuts).
3. **Merge to `main`** through a pull request with CI green (build, tests, MSVC analysis, CodeQL, BinSkim).
4. **Tag.** `git tag vX.Y.Z` on that commit and push the tag. The **Release** workflow checks the tag against `Version.h`, rebuilds with version resources, runs the tests and BinSkim, packs the MSIX, records its SHA-256 and a build-provenance attestation, and opens a **draft** GitHub release.
5. **Sign or submit** (manual, see below), then publish the draft.

To rehearse steps 4's build without tagging, run the Release workflow by hand ("Run workflow"): it produces the same package as a workflow artifact and creates no release.

## Signing

`scripts\package-msix.ps1 -CertificateThumbprint <sha1>` signs the package with a certificate from `Cert:\CurrentUser\My` and timestamps it (RFC 3161). The certificate is referenced by thumbprint so that no password or key file appears on a command line, in the repository or in a log.

- The package's `Publisher` must equal the certificate's subject, exactly.
- Never commit certificates or keys. `.gitignore` excludes `*.pfx` and `*.pvk`; GitHub secret scanning with push protection is enabled on the repository.
- Options for an open-source project: SignPath Foundation (free, applies per project) or a low-cost open-source code-signing certificate. Neither has been applied for; their current terms were not checked.
- When a certificate exists, signing belongs in the Release workflow with the key held in a hardware token or a cloud signing service — not as a file in repository secrets.

## Developer zip

Until there is a certificate, a release carries `ExplorerMate_<version>_x64_unsigned-dev.zip`: the files of the workflow's unsigned MSIX as a loose layout, with `Install.ps1`, `Uninstall.ps1` and a README from `packaging/devzip/`. It registers only with Developer Mode on and is labelled as not for general users.

```powershell
gh run download <release workflow run id> -n ExplorerMate-package-unsigned -D build\devzip\artifact
scripts\package-devzip.ps1 -Msix build\devzip\artifact\package\ExplorerMate_<version>_x64.msix
gh release upload v<version> build\package\ExplorerMate_<version>_x64_unsigned-dev.zip
```

## Store submission

The product is `9PB48F4K2G29` ("Explorer Mate") in Partner Center. The identity it assigned (Product management > Product identity) is recorded in `packaging/store/identity.json`; these values are public, they are part of every installed copy.

1. Build the package to upload:
   ```powershell
   scripts\build.ps1 -Configuration Release -EmbedVersionInfo
   scripts\test.ps1 -Configuration Release -NoBuild
   scripts\package-msix.ps1 -Store
   ```
   Output: `build\package\ExplorerMate_<version>_x64_Store.msix`, unsigned; the Store signs it.
2. In Partner Center start a submission and fill in each section. The text is in `packaging/store/listing.md`: description, features, search terms, the `runFullTrust` justification and the notes for certification. The privacy policy is `PRIVACY.md`, linked by its GitHub URL.
3. Packages: upload the `.msix`. Store listings: paste the text, upload `packaging/store/StoreLogo-300.png` and the screenshots.
4. Pricing: free. Age ratings: answer the questionnaire (no user content, no communication, no purchases).
5. Submit. Certification of a first submission with a restricted capability usually takes a few days.

"Start with Windows" in a packaged install uses the package's startup task (`windows.startupTask` in the release manifest, off by default); the Run registry key is only used by unpackaged development builds, because a packaged program's registry writes are private to the package.

Not verified before a first submission: the Windows App Certification Kit has not been run on the package, and the startup task has only been turned on and off, not observed starting the agent at sign-in.

## Icons

The icon is drawn in two SVG masters under `packaging/icon/` (the full drawing, and a simplified one for 16-24 px). `scripts\build-icons.ps1` renders them with headless Microsoft Edge into `src/App/ExplorerMate.ico`, the package logos in `packaging/Assets/` (including the size-specific "unplated" variants picked up through `resources.pri`) and `packaging/store/StoreLogo-300.png` for the Store listing. The generated files are committed; run the script again only after editing an SVG.

## Store screenshots

`scripts\capture-store-screenshots.ps1` creates a demo folder with harmless sample files (`build\store-demo\Da Lat 2026`), opens each dialog on them, captures it, cancels it (nothing is renamed or moved) and places the capture on a 1920x1080 canvas with a caption. The results are in `packaging/store/screenshots/`:

| File | Shows |
|---|---|
| `02-bulk-rename.png` | The Bulk rename dialog with a mask and the preview |
| `03-new-folder.png` | The New folder with selection dialog |
| `04-about.png` | The introduction window |

The context menu and the tray menu only appear on a real click, so those are captured by hand: open the demo folder, select the photos, right-click, and capture with Win+Shift+S. `01-context-menu.png` is reserved for that capture.

## After publishing

- Confirm `winget install` finds the Store listing.
- Keep the symbol files (`.pdb`) from the workflow artifact with the release; they are needed to read crash dumps.
- Start a new "Unreleased" section in `CHANGELOG.md`.
