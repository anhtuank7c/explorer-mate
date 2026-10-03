# Security policy

## Reporting a vulnerability

Please do not open a public issue for a security problem.

Report it privately through GitHub: open the repository's **Security** tab and choose **Report a vulnerability**. Include what you observed, the steps to reproduce it, and the Windows build you used.

You can expect an acknowledgement within a week. This is a personal project maintained in spare time; fixes are made on a best-effort basis.

## Supported versions

Only the latest release (or, before the first release, the `main` branch) receives fixes.

## What the project considers a security issue

Explorer Mate runs as the signed-in user and never elevates. Problems worth reporting include:

- a way to make a command overwrite, delete or move files the user did not select;
- a way for another process or a crafted file name to make Explorer Mate run code or act on files without the user asking;
- keystrokes or file contents being recorded or sent anywhere;
- the shell extension crashing or hanging File Explorer.

Code that already runs as the same user with full rights is outside the threat model: it can do anything the user can, with or without Explorer Mate.

## How the code is checked

- Compiler warnings at `/W4` are errors; MSVC code analysis and CodeQL run in CI.
- Binaries are built with Control Flow Guard, CET shadow-stack compatibility and a restricted DLL search path.
- The project has no third-party runtime dependencies.
