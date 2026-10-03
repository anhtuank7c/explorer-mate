# Releasing Explorer Mate

What is automated, what a person must do, and what is not set up yet.

## Channels

| Channel | Who signs | Status |
|---|---|---|
| Microsoft Store | Microsoft, on submission | Not submitted yet. Needs a Partner Center account and the reserved name; the package identity it assigns goes into `package-msix.ps1 -IdentityName / -Publisher`. |
| WinGet | — | Available through the `msstore` source once the Store listing exists. A manifest in `winget-pkgs` needs the signed GitHub download. |
| GitHub download | The maintainer's certificate | **No certificate yet.** A Store-signed package cannot be redistributed, so this channel needs its own trusted code-signing certificate. |

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

## Store submission

1. In Partner Center, reserve "Explorer Mate" and note the package identity name and publisher.
2. Build the package with those values:
   ```powershell
   scripts\package-msix.ps1 -IdentityName <name> -Publisher "<CN=...>" -PublisherDisplayName "<publisher>"
   ```
3. Upload the unsigned `.msix`; the Store signs it.
4. The listing needs a privacy policy. The facts for it: no network access, no telemetry, logs stay on the device, the keyboard hook only recognises the configured shortcuts and records nothing.
5. The `runFullTrust` capability must be justified: the app is a classic desktop program that performs file operations through the Windows shell and installs a keyboard hook for its shortcuts.

Not done yet for a Store build: "Start with Windows" must use the package startup-task mechanism instead of the Run registry key.

## Icons

The icon is drawn in two SVG masters under `packaging/icon/` (the full drawing, and a simplified one for 16-24 px). `scripts\build-icons.ps1` renders them with headless Microsoft Edge into `src/App/ExplorerMate.ico`, the package logos in `packaging/Assets/` (including the size-specific "unplated" variants picked up through `resources.pri`) and `packaging/store/StoreLogo-300.png` for the Store listing. The generated files are committed; run the script again only after editing an SVG.

## After publishing

- Confirm `winget install` finds the Store listing.
- Keep the symbol files (`.pdb`) from the workflow artifact with the release; they are needed to read crash dumps.
- Start a new "Unreleased" section in `CHANGELOG.md`.
