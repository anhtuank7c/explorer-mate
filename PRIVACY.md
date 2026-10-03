# Privacy policy

Explorer Mate is a utility that runs entirely on your computer. It does not collect, send or share any information.

Last updated: 4 October 2026.

## What the program does not do

- It makes no network connections. There is no telemetry, no analytics, no advertising, no update check and no account.
- It does not read the contents of your files. It moves, renames and copies the items you select, through the same Windows service File Explorer uses.
- It does not record what you type.

## What stays on your computer

- **Settings.** Your keyboard shortcuts and whether shortcuts are enabled, in a small text file in the program's data folder.
- **Log files.** When something goes wrong, the program writes a line to a log file in its data folder so that the problem can be diagnosed. A line can contain the name or path of a file or folder involved in the failed operation. Log files are limited in size and are never sent anywhere; they leave your computer only if you choose to share them, for example in a bug report.
- **Request files.** When you pick a command from the right-click menu, the list of selected items is written to a temporary file that the program reads and deletes immediately. Leftover files are removed after a day.

Uninstalling Explorer Mate removes this data.

## Keyboard shortcuts

To make its shortcuts work, the tray agent is notified by Windows of key presses while it is running. It uses them for one purpose: to recognise the shortcuts you configured, and only while a File Explorer file list has the keyboard focus. Key presses are not stored, logged or transmitted. Exiting the agent from its tray icon, or turning shortcuts off there, stops this.

## Source code

Explorer Mate is open source. Everything stated here can be checked in the code: https://github.com/anhtuank7c/explorer-mate

## Questions

Open an issue at https://github.com/anhtuank7c/explorer-mate/issues
