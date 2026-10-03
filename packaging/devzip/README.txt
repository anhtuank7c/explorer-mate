Explorer Mate - developer build (unsigned)
==========================================

This zip is for developers and testers. The programs in it are NOT signed.

If you just want to use Explorer Mate, install it from the Microsoft Store:
  https://apps.microsoft.com/detail/9PB48F4K2G29
  winget install 9PB48F4K2G29 --source msstore

What you need
  - Windows 11 on x64.
  - Developer Mode on (Settings > System > For developers). Windows only
    registers unsigned packages in Developer Mode.
  - Smart App Control off. It blocks unsigned programs, and it cannot be
    turned back on once turned off, so do not turn it off just for this.

Install
  1. Extract the whole zip to a folder that will stay where it is, for
     example C:\Tools\ExplorerMate. Windows runs the program from there.
  2. In PowerShell, in that folder:
       powershell -ExecutionPolicy Bypass -File .\Install.ps1
  3. Open "Explorer Mate" from the Start menu once.

Uninstall
       powershell -ExecutionPolicy Bypass -File .\Uninstall.ps1
  Then delete the folder.

Do not install this next to the Microsoft Store version: the right-click
menu would show every command twice.

Where the files come from
  ExplorerMate.exe and ExplorerMate.Shell.dll are the ones the Release
  workflow built from the version tag on GitHub. BUILD-INFO.txt names the
  package they were taken from and its SHA-256.

License: MIT, see LICENSE.txt. Third-party notices: THIRD_PARTY_NOTICES.md.
Source and issues: https://github.com/anhtuank7c/explorer-mate
