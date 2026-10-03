# Microsoft Store listing (English)

Text to paste into Partner Center for product `9PB48F4K2G29`. Keep it in step with what the program really does.

## Properties

- **Category:** Utilities & tools. **Subcategory:** File managers.
- **Privacy policy URL:** https://github.com/anhtuank7c/explorer-mate/blob/main/PRIVACY.md
- **Website:** https://github.com/anhtuank7c/explorer-mate
- **Support contact:** https://github.com/anhtuank7c/explorer-mate/issues
- **Pricing:** Free.

## Product name

Explorer Mate

## Short description

Three extra commands for File Explorer on Windows 11: new folder with selection, bulk rename, and duplicate.

## Description

Explorer Mate adds three commands to the File Explorer you already use. No new file manager to learn: select files, right-click, done.

NEW FOLDER WITH SELECTION
Select files or folders, name a folder, and they move into it.

BULK RENAME
Rename many files at once with a name mask, a counter, and search and replace. A live preview shows every new name before anything changes. The mask syntax follows the conventions known from Total Commander: [N] name, [C] counter, [E] extension, [P] parent folder, and character ranges such as [N1-5].

DUPLICATE
Copy the selected items next to the originals, the way Ctrl+C, Ctrl+V does, in one step.

HOW IT WORKS
- The commands are in the right-click menu of File Explorer.
- Optional keyboard shortcuts (Ctrl+Alt+N, Ctrl+Alt+R, Ctrl+Alt+D by default) work while a File Explorer file list has the focus. Change or turn them off from the tray icon.
- A command line, "explorermate", runs the same three commands from any terminal.
- Operations go through the Windows shell, so you get the standard progress window, the standard conflict prompts, and Ctrl+Z to undo.

PRIVATE BY DESIGN
No network access, no telemetry, no account. Explorer Mate is open source under the MIT license.

LIMITS
Windows 11 on x64. Items on local drives; online-only cloud files, links and junctions are not supported. Windows on ARM is not tested.

## What's new in this version

First release.

## Product features

- New folder with selection: move the selected items into a new folder
- Bulk rename with name masks, a counter, search and replace, and a live preview
- Duplicate the selected items in place
- Commands in the File Explorer right-click menu
- Optional keyboard shortcuts, configurable from the tray icon
- Command line: explorermate
- Uses the Windows shell: standard progress, conflict prompts and undo
- No network access, no telemetry; open source

## Search terms (up to 7)

bulk rename, batch rename, file explorer, context menu, duplicate file, new folder with selection, file manager

## Additional information

- **Copyright and trademark info:** Copyright (c) 2026 Tuan Nguyen. MIT License.
- **Developed by:** Tuan Nguyen (anhtuank7c)

## Screenshots

`packaging/store/screenshots/` (1920x1080): `01-context-menu.png` (captured by hand), `02-bulk-rename.png`, `03-new-folder.png`, `04-about.png`. Store logo: `packaging/store/StoreLogo-300.png`.

## Submission options

### Restricted capability: runFullTrust

Explorer Mate is a classic desktop (Win32) program. It needs full trust to: provide File Explorer context-menu commands through a COM shell extension; move, rename and copy the user's selected files through the Windows shell (IFileOperation); and install a low-level keyboard hook for its optional keyboard shortcuts, which act only while a File Explorer file list has the focus. It makes no network connections.

### Notes for certification

1. Install, then open "Explorer Mate" from the Start menu once. An introduction window opens and a tray icon appears.
2. In File Explorer, open any folder with a few files, select two or more files and right-click. The commands "New folder with selection", "Bulk rename" and "Duplicate" are in the menu (Windows may group them under "Explorer Mate").
3. Keyboard shortcuts: with files selected and the file list focused, press Ctrl+Alt+D to duplicate, Ctrl+Alt+N for a new folder, Ctrl+Alt+R for bulk rename.
4. Tray icon: right-click for settings, "Start with Windows", "What's new", "About" and "Exit".

No account or sign-in is needed. The app does not use the network.
