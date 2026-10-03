# Code signing through SignPath Foundation

Draft of the application for free open-source code signing, and what has to be in place. The conditions below are from https://signpath.org/terms as read on 4 October 2026; check the page again before applying, and follow whatever the application form asks for where it differs.

Why: a package downloaded from GitHub only installs if it is signed by a certificate Windows trusts. The Microsoft Store signs its own copy, which cannot be redistributed.

## Where the project stands

| Condition | Status |
|---|---|
| OSI-approved licence, no commercial dual licensing | Met: MIT. |
| No proprietary components | Met: only the Microsoft runtime and SDK, which count as system libraries. See `THIRD_PARTY_NOTICES.md`. |
| Actively maintained | Met. |
| Already released in the form to be signed | Partly: v0.1.0 is released and the Release workflow builds the MSIX, but only as an unsigned developer zip and workflow artifact. |
| Functionality documented on the download page | Met: README and the release notes. |
| Binaries from verifiable, automated builds of the repository | Met: the Release workflow on GitHub-hosted runners, with a build-provenance attestation. |
| Product name and version in the binaries' metadata | Met in release builds (`build.ps1 -EmbedVersionInfo`): both the EXE and the DLL carry them. |
| Every signing request approved by a person | To set up in SignPath when accepted. |
| Multi-factor authentication for the repository and SignPath | **To do by the maintainer:** make sure two-factor authentication is on for the GitHub account. |
| "Code signing policy" on the project page | **To add** (text below) once SignPath confirms, since it names them as the provider. |
| Privacy policy | Met: `PRIVACY.md`. |
| Uninstall instructions | Met by MSIX: Settings > Apps > Installed apps. Add one line to the README. |
| No hacking tools, respects user privacy, announces system changes | Met. Worth stating in the application: the keyboard hook only recognises the configured shortcuts while a File Explorer file list has the focus, and records nothing. |

## Application text

Apply at https://signpath.org/apply (the form itself was not readable when this was written; adapt the text to its fields).

- **Project name:** Explorer Mate
- **Repository:** https://github.com/anhtuank7c/explorer-mate
- **Homepage:** https://github.com/anhtuank7c/explorer-mate
- **Download page:** https://github.com/anhtuank7c/explorer-mate/releases
- **Licence:** MIT
- **Maintainer:** Tuan Nguyen (GitHub: anhtuank7c)
- **Build system:** GitHub Actions, workflow `.github/workflows/release.yml`, GitHub-hosted runner `windows-2025-vs2026`
- **Artifact to sign:** one MSIX package per release, `ExplorerMate_<version>_x64.msix`, containing `ExplorerMate.exe` and `ExplorerMate.Shell.dll`

**Description**

> Explorer Mate is a small native C++ utility for Windows 11 that adds three commands to File Explorer: move the selected items into a new folder, rename many files at once with a name mask and a live preview, and duplicate the selected items in place. The commands appear in the right-click menu, as optional keyboard shortcuts, and as a command line. File operations go through the Windows shell (IFileOperation), so the standard progress, conflict prompts and undo apply.
>
> The program makes no network connections and collects no data. Its optional keyboard shortcuts use a low-level keyboard hook that only recognises the configured shortcuts while a File Explorer file list has the keyboard focus; key presses are not stored or transmitted. The source of this is `src/Infrastructure/KeyboardHook.cpp` and `src/Application/HotkeyMatcher.cpp`.
>
> The project is published in the Microsoft Store, which signs its own copy. We are asking for code signing so that the same MSIX package can also be offered as a direct download from GitHub releases and through the community WinGet repository. Today the GitHub release can only carry an unsigned build that installs with Developer Mode.
>
> Releases are built by GitHub Actions from a version tag: the workflow checks the tag against the version in the source, builds, runs the tests and Microsoft BinSkim, packs the MSIX and records a build-provenance attestation. CI on every push also runs MSVC code analysis and CodeQL. The project has a single maintainer, who would hold the author, reviewer and approver roles.

## "Code signing policy" for the README

To be added when SignPath accepts the project.

```markdown
## Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org).

- Committers and reviewers: [Tuan Nguyen (anhtuank7c)](https://github.com/anhtuank7c)
- Approvers: [Tuan Nguyen (anhtuank7c)](https://github.com/anhtuank7c)

Privacy policy: [PRIVACY.md](PRIVACY.md). This program will not transfer any information to other networked systems unless specifically requested by the user or the person installing or operating it.
```

## After acceptance

1. Create the SignPath project, artifact configuration (MSIX) and signing policy as their onboarding describes.
2. The package's `Publisher` must equal the subject of the SignPath certificate exactly; pass it to `package-msix.ps1 -Publisher`. This gives the GitHub package a different identity from the Store package, so the two cannot be installed side by side.
3. In `release.yml`, after "Package", submit the MSIX to SignPath with their GitHub action, wait for the approved signed package, and attach that to the draft release instead of only the developer zip. Keep the API token in a repository secret; pin the action to a commit like the others.
4. Update `docs/RELEASING.md`, the README install section and the release notes; then a manifest can be submitted to the community WinGet repository.
