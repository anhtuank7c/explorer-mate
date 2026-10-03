# Third-party notices

Explorer Mate itself is released under the MIT License (see LICENSE). This file lists everything in the program, or used to build it, that comes from someone else. It is shown in the program under About > Licenses and is shipped inside the package.

Explorer Mate has no third-party source code in its repository and uses no package manager. Check this file again whenever a dependency, icon, font or tool is added.

## Included in the program

### Lucide icons (ISC License)

The three context-menu command icons are from Lucide (https://lucide.dev), version 1.51.0: `folder-plus`, `pen-line` and `copy`. The SVG sources are in `packaging/icon/menu/`; `scripts/build-icons.ps1` renders them, recoloured for the light and dark menu, into `src/ShellExtension/icons/*.ico`, which are compiled into `ExplorerMate.Shell.dll`.

None of the three is among the icons that Lucide's license lists as derived from the Feather project, so only the ISC License applies.

```
ISC License

Copyright (c) 2026 Lucide Icons and Contributors

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
```

### Microsoft C++ Standard Library (Apache License 2.0 with LLVM exception)

The program is built with the Microsoft Visual C++ toolset and links the C++ Standard Library statically. Its source is published at https://github.com/microsoft/STL under the Apache License 2.0 with the LLVM exception. The exception removes the attribution requirement for compiled code embedded in a program; it is listed here for completeness.

### Microsoft Visual C++ runtime, Windows Runtime C++ Template Library and Windows SDK (not open source)

The C runtime is linked statically, and the program uses the Windows Runtime C++ Template Library (WRL) and the Windows SDK headers and import libraries. These are licensed by Microsoft under the Visual Studio and Windows SDK license terms, which allow the compiled code to be distributed as part of an application.

## Used to build and test, not distributed

Nothing in this section is part of the program or of its package.

- GitHub Actions `actions/checkout`, `actions/upload-artifact` and `actions/attest-build-provenance`: MIT License.
- `github/codeql-action` (CodeQL code scanning): MIT License. The CodeQL engine it runs has its own terms, which allow use on open-source repositories.
- Microsoft BinSkim (binary hardening check, downloaded by `scripts/check-binaries.ps1`): MIT License.
- Microsoft C++ Unit Test Framework: part of Visual Studio, under its license terms.
- Microsoft Edge (headless) renders the icons from SVG in `scripts/build-icons.ps1`; only the resulting images are kept.

## Original work

- The application icon (`packaging/icon/ExplorerMate*.svg`) and the logos generated from it were made for this project.
- The bulk-rename mask syntax follows the conventions of Total Commander's Multi-Rename Tool. It is an independent implementation; no code or artwork from Total Commander is used.
- `CHANGELOG.md` follows the Keep a Changelog format. This is a convention, not included material.
