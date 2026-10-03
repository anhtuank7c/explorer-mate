# Changelog

All notable changes to Explorer Mate are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and versions follow
[Semantic Versioning](https://semver.org/).

## [0.1.0] - 2026-10-04

First release.

### Added

- Context-menu commands for the Windows 11 File Explorer: New folder with selection, Bulk rename, Duplicate.
- Bulk rename with Total Commander style masks (`[N]`, `[E]`, `[C]`, `[P]`, character ranges), counter settings, search and replace, and a live preview.
- Keyboard shortcuts through a tray agent, active only in Explorer's file list; configurable from the tray icon.
- "Start with Windows" for the tray agent, from the tray menu or with `explorermate --autostart on`. Opening Explorer Mate from the Start menu starts the agent.
- `explorermate` command in any terminal for the three commands, with `--help` and item paths relative to the current folder.
- Introduction window with links to the source code and the issue tracker, a "What's new" window showing this changelog and a "Licenses" window with the third-party notices.

### Security

- Release binaries are built with Control Flow Guard, CET shadow-stack compatibility and a System32-only DLL search path for imports.
- CI runs MSVC code analysis, CodeQL and BinSkim.
