# Changelog

All notable changes to Explorer Mate are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and versions follow
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- Context-menu commands for the Windows 11 File Explorer: New folder with selection, Bulk rename, Duplicate.
- Bulk rename with Total Commander style masks (`[N]`, `[E]`, `[C]`, `[P]`, character ranges), counter settings, search and replace, and a live preview.
- Keyboard shortcuts through a tray agent, active only in Explorer's file list; configurable from the tray icon.
- Command line for the three commands, an introduction window, and Explorer diagnostics.
- Development install through a sparse package; release-style MSIX packaging script.

### Fixed

- File Explorer now shows the result of a command immediately; before, the file list could keep showing moved files until refreshed with F5.

### Security

- Release binaries are built with Control Flow Guard, CET shadow-stack compatibility and a System32-only DLL search path for imports.
- CI runs MSVC code analysis, CodeQL and BinSkim.
