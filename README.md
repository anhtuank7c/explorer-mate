# Explorer Mate

**English** · [Tiếng Việt](#tiếng-việt)

A small companion for the Windows 11 File Explorer. It adds three commands that Explorer does not have, right where you already work: the right-click menu and the keyboard.

| Command | What it does | Default shortcut |
|---|---|---|
| **New folder with selection** | Creates a folder next to the selected items and moves them into it | Ctrl+Alt+N |
| **Bulk rename** | Renames many files at once with a name mask, a counter and a live preview | Ctrl+Alt+R |
| **Duplicate** | Copies the selected files or folders next to the originals | Ctrl+Alt+D |

Explorer Mate is not a file manager and does not replace Explorer. You keep Explorer, its tabs and everything you are used to; you just get a few more things to do with a selection.

## Install

- **Microsoft Store:** [Explorer Mate](https://apps.microsoft.com/detail/9PB48F4K2G29)
- **WinGet:** `winget install 9PB48F4K2G29 --source msstore`

After installing, open Explorer Mate from the Start menu once: that starts the tray agent for the keyboard shortcuts. To uninstall, use Settings > Apps > Installed apps.

> **Status: version 0.1.0, the first release.** Windows 11 on x64 only. A signed download from GitHub is not available yet; the [release page](https://github.com/anhtuank7c/explorer-mate/releases) carries an unsigned build for developers.

## Why

On macOS, Finder lets you select a few files and choose "New Folder with Selection", or duplicate something in place with one keystroke. On Windows the same jobs take several steps. Explorer Mate brings those two actions over and adds a bulk rename modelled on the Multi-Rename Tool of Total Commander.

## Features

- **Lives in the Windows 11 context menu** — the main one, not "Show more options".
- **Keyboard shortcuts that only act inside Explorer's file list.** In the search box, the address bar, a rename box or any other app, the keys behave normally. Shortcuts can be changed or turned off from the tray icon.
- **Bulk rename with masks:**

  | Placeholder | Meaning |
  |---|---|
  | `[N]` / `[E]` | Original name (without extension) / original extension |
  | `[C]` | Counter, with start, step and number of digits |
  | `[P]` | Name of the parent folder |
  | `[N2-5]`, `[N2-]`, `[N2]`, `[N2,3]` | Characters 2 to 5; from character 2; character 2 only; 3 characters from position 2 |
  | `[[]`, `[]]` | Literal square brackets |

  The default mask `[N]_[C]` turns `photo.jpg` into `photo_01.jpg`. `Trip_[C]` turns `IMG_7.jpg`, `IMG_12.jpg` into `Trip_01.jpg`, `Trip_02.jpg`. There is also search-and-replace, and the dialog shows every new name before you confirm.
- **Careful with your files.** No command overwrites or merges into something that already exists. A rename batch is checked as a whole before the first file is touched. Operations go through the Windows shell, so you get the standard progress window.
- **Small.** One menu DLL and one EXE that runs only when needed, plus an optional tray agent for the shortcuts. No telemetry, no network access.

## What it does not do

- It only works on regular folders on local drives. Network paths, online-only OneDrive files, links/junctions, ZIP contents and the Recycle Bin are refused with a message.
- All selected items must be in the same folder.
- Bulk rename works on files, not folders.
- Windows 11 on x64 (Intel/AMD) only. Windows 10 is not supported.
- **Windows on ARM is not supported.** There is no ARM64 build, and the maintainer has no ARM device to test one on. See [Help wanted: Windows on ARM](#help-wanted-windows-on-arm).
- Compared with Total Commander's rename tool, date/time placeholders, case conversion, regular expressions and saved presets are not there yet.

## Using it

Select files or folders in File Explorer and right-click; Windows may group the three commands under "Explorer Mate". With the tray agent running, the shortcuts in the table above work while the file list has the keyboard focus. Click the tray icon to change shortcuts, toggle "Start with Windows" or exit.

Opening Explorer Mate from the Start menu shows an introduction window. There is also a command line: once installed, `explorermate` works in any terminal, with paths relative to the current folder.

```powershell
explorermate --action duplicate report.docx Assets
explorermate --action group --name "Project A" a.txt b.jpg
explorermate --action rename --mask "Trip_[C]" IMG_7.jpg IMG_12.jpg
explorermate --help
```

Without `--name` or a rename option the usual dialog asks for it; `--silent` runs with no windows at all. Wildcards such as `*.jpg` are not expanded.

If a command does nothing, check the logs in `%LOCALAPPDATA%\ExplorerMate\logs`.

## For developers

Everything below is what you need to clone the project, run it and change it.

### Prerequisites

- Windows 11 x64.
- Visual Studio 18 with the "Desktop development with C++" workload (toolset **v145**) and the Windows 11 SDK. The C++ unit-test framework that ships with Visual Studio is used; nothing else needs installing.
- **Developer Mode** on (Settings → System → For developers). The context menu needs a registered package, and unsigned packages can only be registered in Developer Mode.
- Git, and Windows PowerShell 5.1 (included with Windows).

A different Visual Studio version means changing `PlatformToolset` in `Directory.Build.props`.

### Clone, build, test, run

```powershell
git clone https://github.com/anhtuank7c/explorer-mate.git
cd explorer-mate

# Build Debug and Release (x64)
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration All

# Layer check + build + all tests
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test.ps1 -Configuration Debug

# Register the context menu for your user and start the shortcut agent
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\install-dev.ps1 -StartAgent

# Remove it again
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\uninstall-dev.ps1
```

You can also open `ExplorerMate.sln` in Visual Studio; tests show up in Test Explorer. After changing the DLL or EXE, run `install-dev.ps1` again so the menu uses the new binaries (it stops and restarts the agent for you).

If **Smart App Control** is on, Windows may refuse to run an unsigned build ("An Application Control policy has blocked this file"). The verdict differs from one build to the next; deleting `build\obj\App` and rebuilding usually produces a binary that is accepted.

### Project layout

```text
src/Domain/            pure rules, standard library only        (namespace et::domain)
src/Application/       use cases and ports (interfaces)          (et::app)
src/Infrastructure/    Win32/COM adapters implementing the ports (et::infra)
src/App/               ExplorerMate.exe: dialogs, CLI, tray agent (et::ui)
src/ShellExtension/    ExplorerMate.Shell.dll: context-menu commands
tests/UnitTests/       Domain + Application, in-memory fakes, no disk access
tests/IntegrationTests/ adapters against real files in %TEMP%\ExplorerMate.Tests
packaging/             package manifests (development and release) and logos
scripts/               build, test, install, packaging and probe scripts
docs/                  specification, architecture, test matrix, progress log
```

Dependencies point inward only: `App`/`ShellExtension` → `Infrastructure` → `Application` → `Domain`. `scripts\check-layers.ps1` (run by `test.ps1`) fails when `Domain` or `Application` include a Windows header or an outer layer.

| Document | Language | Content |
|---|---|---|
| [`docs/SRS.md`](docs/SRS.md) | English | Requirements with IDs and how each is verified |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | English | Processes, layers, sequence diagrams, safety rules, decisions |
| [`docs/TEST_MATRIX.md`](docs/TEST_MATRIX.md) | Vietnamese | What has been tested, how, and what has not |
| [`docs/PROGRESS.md`](docs/PROGRESS.md) | Vietnamese | Milestone log with measured Windows behaviour |

### Tests and tools

| What | How |
|---|---|
| Unit + integration tests | `scripts\test.ps1 -Configuration Debug` (`-NoBuild` to skip the build) |
| Invoke a registered menu command without the GUI | `& .\scripts\probe-command.ps1 -Clsid <clsid> -Path <item1>,<item2>` (CLSIDs are in `packaging\AppxManifest.xml`) |
| Drive a dialog without a person | `& .\scripts\probe-dialog.ps1 -Action group\|rename -Path <items> [-SetText @{<id>='text'}] [-Press ok]` |
| Check tab and focus detection | `scripts\test-tab-detection.ps1` (opens and closes its own Explorer window) |
| See what a shortcut would act on right now | `ExplorerMate.exe --diagnose-explorer \| Out-String` |
| Static analysis (MSVC `/analyze`, findings are errors) | `scripts\build.ps1 -Configuration Debug -Analyze` |
| Check exploit mitigations in the binaries (BinSkim) | `scripts\check-binaries.ps1` (downloads BinSkim into `build\tools` on first use) |
| Build the release-style MSIX | `scripts\package-msix.ps1` (unsigned unless you pass `-CertificateThumbprint`) |
| Regenerate the icon files after editing `packaging\icon\*.svg` | `scripts\build-icons.ps1` (needs Microsoft Edge; the generated files are committed) |

CI on GitHub runs the build, the tests, MSVC analysis, CodeQL and BinSkim on every push and pull request (`.github/workflows`). How a release is cut is in [`docs/RELEASING.md`](docs/RELEASING.md); how to report a vulnerability is in [`SECURITY.md`](SECURITY.md); user-visible changes go in [`CHANGELOG.md`](CHANGELOG.md).

The EXE is a GUI-subsystem program: in PowerShell, pipe it (`| Out-String`) to wait for it and see its output.

### Debugging

- Logs: `%LOCALAPPDATA%\ExplorerMate\logs\shell.log` (menu DLL) and `agent.log` (shortcuts; it records why a shortcut was refused).
- The menu DLL runs in `DllHost.exe`, not in `explorer.exe`. To debug it, attach to the `DllHost.exe` whose command line contains one of the command CLSIDs.
- The worker is the easiest thing to debug: start `ExplorerMate.exe --action …` under the debugger with test paths; add `--silent` to skip shell UI.
- To reproduce a rename or naming bug, write a failing test in `tests/UnitTests` first; those rules have no Windows dependency.

### Adding a command

1. **Rules** — put pure logic in `src/Domain` with unit tests.
2. **Action** — add a value to `ActionKind` and its wire name in `ActionKind.cpp`.
3. **Use case** — add a class in `src/Application` with `Execute(paths)`. If it needs something from Windows, add a method to a port (or a new port), implement it in `src/Infrastructure` and in `tests/UnitTests/Fakes.h`.
4. **Wiring** — add a case in `src/App/ActionRunner.cpp`.
5. **Menu** — add a class in `src/ShellExtension/Commands.cpp` with a new CLSID, list it in both `packaging/AppxManifest.xml` and `packaging/release/AppxManifest.xml`, and add it to `ShellExtensionActivationTests.cpp`.
6. **Shortcut** (optional) — add it to `Settings::Defaults()` and `kAllActions` in `Settings.cpp`, and to `SettingsDialog.cpp` plus the dialog in `App.rc`.
7. **Project files** — new source files must be added to the matching `.vcxproj` by hand.

Existing use cases do not need to change.

### Conventions

- `/std:c++20 /W4 /WX /permissive- /utf-8`, static CRT, x64 only. Shared settings live in `Directory.Build.props` and `Directory.Build.targets`, not in each project.
- Include from the `src` root: `#include "Domain/Result.h"`.
- Return errors with `et::domain::Result<T>`; no exception may cross a COM boundary.
- RAII for every Windows/COM resource; no bare `new`/`delete`.
- Never use `FOF_NOCONFIRMATION`, and never loosen the tab-selection gate in `ExplorerSelectionSource` — both exist to protect user files.
- Do not name methods after `windows.h` macros (`CreateDirectory`, `MoveFile`, `CopyFile`, `DeleteFile`…).
- Scripts must run on Windows PowerShell 5.1 and stay ASCII-only.
- Tests may only touch files inside `%TEMP%\ExplorerMate.Tests`.

### Help wanted: Windows on ARM

Explorer Mate is only built and tested for x64. Supporting Windows on ARM needs someone with an ARM device, because the parts that matter most cannot be checked any other way. Nothing in the code is known to be x64-specific, so the work is expected to be mostly configuration and testing:

- Add an ARM64 platform to `ExplorerMate.sln`, the projects and `Directory.Build.props`/`.targets` (CET, `/CETCOMPAT`, is x64-only and must be left out), and a platform parameter to the scripts.
- Build an ARM64 MSIX (`ProcessorArchitecture="arm64"` in the manifests).
- Add a CI job on GitHub's `windows-11-vs2026-arm` runner so the ARM64 build and tests run natively.
- **On a real ARM device:** confirm the three commands appear in the context menu and work, the dialogs open in front, and the keyboard shortcuts act on the right tab (`scripts\test-tab-detection.ps1` checks the tab and focus detection, which depends on Explorer's window structure).

If you can help, open an issue or a pull request and say which device and Windows build you tested on.

### Contributing

Issues and pull requests are welcome. Before opening a pull request, run `scripts\test.ps1` on both Debug and Release, add or update tests for the behaviour you changed, and say in the description what you tested by hand (the menu and shortcuts cannot be fully covered by automated tests).

## Credits

The idea, the requirements and the hands-on testing are by **Tuan Nguyen** ([@anhtuank7c](https://github.com/anhtuank7c), [meohamhoc.vn](https://meohamhoc.vn)). The code and documentation were written with [Claude Code](https://claude.com/claude-code), Anthropic's AI coding agent, working from those requirements. This is not a hand-written codebase, and it is presented as what it is.

The bulk rename syntax is inspired by Total Commander; "New folder with selection" and "Duplicate" by macOS Finder. Explorer Mate is not affiliated with either, nor with Microsoft.

## Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io), certificate by [SignPath Foundation](https://signpath.org).

> Status: the application to SignPath Foundation is pending. Until it is accepted, the downloads on GitHub are **not** signed; the Microsoft Store signs the copy it distributes.

- Committers and reviewers: [Tuan Nguyen (anhtuank7c)](https://github.com/anhtuank7c)
- Approvers: [Tuan Nguyen (anhtuank7c)](https://github.com/anhtuank7c)

Only packages built by the [Release workflow](.github/workflows/release.yml) from a version tag of this repository are submitted for signing, and each request is approved by hand.

Privacy policy: [PRIVACY.md](PRIVACY.md). This program will not transfer any information to other networked systems unless specifically requested by the user or the person installing or operating it.

To uninstall: Settings > Apps > Installed apps > Explorer Mate > Uninstall.

## License

[MIT](LICENSE) © 2026 Tuan Nguyen.

---

# Tiếng Việt

[English](#explorer-mate) · **Tiếng Việt**

Một người bạn đồng hành nhỏ cho File Explorer trên Windows 11. Công cụ thêm ba lệnh mà Explorer chưa có, đặt ngay ở nơi bạn vẫn thao tác: menu chuột phải và bàn phím.

| Lệnh | Tác dụng | Phím tắt mặc định |
|---|---|---|
| **New folder with selection** | Tạo thư mục mới ngay tại chỗ và chuyển các mục đang chọn vào đó | Ctrl+Alt+N |
| **Bulk rename** | Đổi tên nhiều file cùng lúc bằng mask, bộ đếm và bảng xem trước | Ctrl+Alt+R |
| **Duplicate** | Nhân bản file hoặc thư mục đang chọn ngay cạnh bản gốc | Ctrl+Alt+D |

Explorer Mate không phải trình quản lý file và không thay thế Explorer. Bạn vẫn dùng Explorer, các tab và mọi thứ quen thuộc; chỉ có thêm vài việc làm được với những gì đang chọn.

## Cài đặt

- **Microsoft Store:** [Explorer Mate](https://apps.microsoft.com/detail/9PB48F4K2G29)
- **WinGet:** `winget install 9PB48F4K2G29 --source msstore`

Cài xong, hãy mở Explorer Mate từ Start menu một lần để bật agent ở khay cho các phím tắt. Muốn gỡ, vào Settings > Apps > Installed apps.

> **Trạng thái: phiên bản 0.1.0, bản phát hành đầu tiên.** Chỉ hỗ trợ Windows 11 x64. Chưa có bản tải đã ký trên GitHub; [trang release](https://github.com/anhtuank7c/explorer-mate/releases) chỉ có bản chưa ký dành cho developer.

## Vì sao có công cụ này

Trên macOS, Finder cho phép chọn vài file rồi bấm "New Folder with Selection", hoặc nhân bản tại chỗ bằng một phím tắt. Trên Windows, những việc đó mất vài bước. Explorer Mate mang hai thao tác ấy sang, và thêm tính năng đổi tên hàng loạt theo kiểu Multi-Rename Tool của Total Commander.

## Tính năng

- **Nằm trong menu chuột phải của Windows 11** — menu chính, không phải "Show more options".
- **Phím tắt chỉ có tác dụng trong danh sách file của Explorer.** Ở ô tìm kiếm, thanh địa chỉ, ô đổi tên hay ứng dụng khác, phím hoạt động như bình thường. Có thể đổi hoặc tắt phím tắt từ icon ở khay hệ thống.
- **Đổi tên hàng loạt bằng mask:**

  | Placeholder | Ý nghĩa |
  |---|---|
  | `[N]` / `[E]` | Tên gốc (không gồm đuôi) / đuôi gốc |
  | `[C]` | Bộ đếm, chỉnh được số bắt đầu, bước nhảy và số chữ số |
  | `[P]` | Tên thư mục chứa file |
  | `[N2-5]`, `[N2-]`, `[N2]`, `[N2,3]` | Ký tự 2 đến 5; từ ký tự 2; chỉ ký tự 2; 3 ký tự từ vị trí 2 |
  | `[[]`, `[]]` | Dấu ngoặc vuông nguyên văn |

  Mask mặc định `[N]_[C]` đổi `anh.jpg` thành `anh_01.jpg`. Mask `DaLat_[C]` đổi `IMG_7.jpg`, `IMG_12.jpg` thành `DaLat_01.jpg`, `DaLat_02.jpg`. Có thêm tìm–thay thế, và hộp thoại hiện toàn bộ tên mới trước khi bạn xác nhận.
- **Cẩn thận với dữ liệu của bạn.** Không lệnh nào ghi đè hay gộp vào mục đã tồn tại. Một lô đổi tên được kiểm tra toàn bộ trước khi đụng tới file đầu tiên. Thao tác đi qua Windows shell nên bạn có cửa sổ tiến trình quen thuộc.
- **Nhỏ gọn.** Một DLL cho menu và một EXE chỉ chạy khi cần, cùng một agent ở khay hệ thống (tùy chọn) cho phím tắt. Không thu thập dữ liệu, không kết nối mạng.

## Những gì chưa làm được

- Chỉ hoạt động với thư mục thông thường trên ổ đĩa cục bộ. Đường dẫn mạng, file OneDrive chỉ-trên-mây, link/junction, nội dung ZIP và Recycle Bin bị từ chối kèm thông báo.
- Các mục được chọn phải nằm chung một thư mục.
- Bulk rename áp dụng cho file, không áp dụng cho thư mục.
- Chỉ hỗ trợ Windows 11 trên máy x64 (Intel/AMD); không hỗ trợ Windows 10.
- **Không hỗ trợ Windows trên ARM.** Dự án chưa có bản build ARM64, và tác giả không có thiết bị ARM để thử. Xem [Cần cộng đồng giúp: Windows trên ARM](#cần-cộng-đồng-giúp-windows-trên-arm).
- So với công cụ đổi tên của Total Commander, hiện chưa có placeholder ngày giờ, đổi hoa/thường, biểu thức chính quy và lưu mẫu đặt tên.

## Cách dùng

Chọn file hoặc thư mục trong File Explorer rồi bấm chuột phải; Windows có thể gom ba lệnh vào mục "Explorer Mate". Khi agent ở khay đang chạy, các phím tắt trong bảng trên hoạt động lúc danh sách file đang giữ con trỏ bàn phím. Bấm icon ở khay để đổi phím tắt, bật/tắt "Start with Windows" hoặc thoát.

Mở Explorer Mate từ Start menu sẽ hiện cửa sổ giới thiệu. Ngoài ra có dòng lệnh: sau khi cài, gõ `explorermate` trong bất kỳ terminal nào, đường dẫn có thể tương đối so với thư mục hiện tại.

```powershell
explorermate --action duplicate report.docx Assets
explorermate --action group --name "Project A" a.txt b.jpg
explorermate --action rename --mask "DaLat_[C]" IMG_7.jpg IMG_12.jpg
explorermate --help
```

Không truyền `--name` hoặc tùy chọn đổi tên thì hộp thoại quen thuộc sẽ hỏi; `--silent` chạy mà không hiện cửa sổ nào. Ký tự đại diện như `*.jpg` chưa được bung.

Nếu bấm lệnh mà không có gì xảy ra, xem log trong `%LOCALAPPDATA%\ExplorerMate\logs`.

## Dành cho developer

Phần dưới đây là những gì bạn cần để clone dự án, chạy và sửa đổi.

### Yêu cầu

- Windows 11 x64.
- Visual Studio 18 với workload "Desktop development with C++" (toolset **v145**) và Windows 11 SDK. Dự án dùng bộ unit test C++ đi kèm Visual Studio, không cần cài thêm gì.
- Bật **Developer Mode** (Settings → System → For developers). Menu chuột phải cần một package đã đăng ký, và package chưa ký chỉ đăng ký được khi bật Developer Mode.
- Git và Windows PowerShell 5.1 (có sẵn trong Windows).

Nếu dùng phiên bản Visual Studio khác, đổi `PlatformToolset` trong `Directory.Build.props`.

### Clone, build, test, chạy

```powershell
git clone https://github.com/anhtuank7c/explorer-mate.git
cd explorer-mate

# Build Debug và Release (x64)
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration All

# Kiểm tra lớp + build + toàn bộ test
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test.ps1 -Configuration Debug

# Đăng ký menu chuột phải cho tài khoản của bạn và bật agent phím tắt
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\install-dev.ps1 -StartAgent

# Gỡ ra
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\uninstall-dev.ps1
```

Bạn cũng có thể mở `ExplorerMate.sln` bằng Visual Studio; test hiện trong Test Explorer. Sau khi sửa DLL hoặc EXE, chạy lại `install-dev.ps1` để menu dùng bản mới (script tự dừng và chạy lại agent).

Nếu máy bật **Smart App Control**, Windows có thể từ chối chạy bản build chưa ký ("An Application Control policy has blocked this file"). Phán quyết khác nhau giữa các lần build; xóa `build\obj\App` rồi build lại thường cho ra file được chấp nhận.

### Cấu trúc dự án

```text
src/Domain/            luật thuần, chỉ dùng thư viện chuẩn        (namespace et::domain)
src/Application/       use case và port (interface)              (et::app)
src/Infrastructure/    adapter Win32/COM hiện thực các port       (et::infra)
src/App/               ExplorerMate.exe: dialog, CLI, agent ở khay (et::ui)
src/ShellExtension/    ExplorerMate.Shell.dll: lệnh menu chuột phải
tests/UnitTests/       Domain + Application, dùng fake trong bộ nhớ, không chạm ổ đĩa
tests/IntegrationTests/ adapter chạy trên file thật trong %TEMP%\ExplorerMate.Tests
packaging/             manifest package (bản dev và bản phát hành) và logo
scripts/               script build, test, cài đặt, đóng gói và kiểm tra
docs/                  đặc tả, kiến trúc, ma trận kiểm thử, nhật ký tiến độ
```

Phụ thuộc chỉ đi vào trong: `App`/`ShellExtension` → `Infrastructure` → `Application` → `Domain`. `scripts\check-layers.ps1` (được `test.ps1` gọi) báo lỗi khi `Domain` hoặc `Application` include header của Windows hay lớp ngoài.

| Tài liệu | Ngôn ngữ | Nội dung |
|---|---|---|
| [`docs/SRS.md`](docs/SRS.md) | Tiếng Anh | Yêu cầu có mã định danh và cách kiểm chứng từng yêu cầu |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Tiếng Anh | Tiến trình, các lớp, sequence diagram, nguyên tắc an toàn, quyết định thiết kế |
| [`docs/TEST_MATRIX.md`](docs/TEST_MATRIX.md) | Tiếng Việt | Đã kiểm thử gì, bằng cách nào, và những gì chưa kiểm |
| [`docs/PROGRESS.md`](docs/PROGRESS.md) | Tiếng Việt | Nhật ký theo milestone, kèm hành vi Windows đã đo được |

### Test và công cụ

| Việc | Cách làm |
|---|---|
| Unit test + integration test | `scripts\test.ps1 -Configuration Debug` (thêm `-NoBuild` để bỏ qua build) |
| Gọi một lệnh menu đã đăng ký mà không cần giao diện | `& .\scripts\probe-command.ps1 -Clsid <clsid> -Path <item1>,<item2>` (CLSID nằm trong `packaging\AppxManifest.xml`) |
| Điều khiển dialog không cần người bấm | `& .\scripts\probe-dialog.ps1 -Action group\|rename -Path <items> [-SetText @{<id>='text'}] [-Press ok]` |
| Kiểm tra nhận diện tab và focus | `scripts\test-tab-detection.ps1` (tự mở và đóng một cửa sổ Explorer riêng) |
| Xem phím tắt lúc này sẽ tác động lên gì | `ExplorerMate.exe --diagnose-explorer \| Out-String` |
| Phân tích tĩnh (MSVC `/analyze`, mọi phát hiện đều là lỗi) | `scripts\build.ps1 -Configuration Debug -Analyze` |
| Kiểm tra các biện pháp chống khai thác trong file nhị phân (BinSkim) | `scripts\check-binaries.ps1` (tự tải BinSkim vào `build\tools` ở lần chạy đầu) |
| Dựng gói MSIX kiểu phát hành | `scripts\package-msix.ps1` (chưa ký, trừ khi truyền `-CertificateThumbprint`) |
| Sinh lại các file icon sau khi sửa `packaging\icon\*.svg` | `scripts\build-icons.ps1` (cần Microsoft Edge; file sinh ra đã được commit sẵn) |

CI trên GitHub chạy build, test, phân tích MSVC, CodeQL và BinSkim cho mỗi lần push và pull request (`.github/workflows`). Quy trình phát hành nằm ở [`docs/RELEASING.md`](docs/RELEASING.md); cách báo lỗ hổng bảo mật ở [`SECURITY.md`](SECURITY.md); thay đổi người dùng nhìn thấy được ghi vào [`CHANGELOG.md`](CHANGELOG.md).

EXE là chương trình GUI-subsystem: trong PowerShell phải đưa vào pipeline (`| Out-String`) thì mới chờ nó chạy xong và thấy output.

### Gỡ lỗi

- Log: `%LOCALAPPDATA%\ExplorerMate\logs\shell.log` (DLL menu) và `agent.log` (phím tắt; có ghi lý do một phím tắt bị từ chối).
- DLL menu chạy trong `DllHost.exe`, không phải `explorer.exe`. Muốn debug thì attach vào `DllHost.exe` có dòng lệnh chứa một trong các CLSID của lệnh.
- Worker dễ debug nhất: chạy `ExplorerMate.exe --action …` dưới debugger với đường dẫn thử; thêm `--silent` để bỏ giao diện của shell.
- Với lỗi đổi tên hay đặt tên, hãy viết trước một test thất bại trong `tests/UnitTests`; các luật đó không phụ thuộc Windows.

### Thêm một lệnh mới

1. **Luật** — đặt logic thuần vào `src/Domain`, kèm unit test.
2. **Action** — thêm giá trị vào `ActionKind` và tên truyền (wire name) trong `ActionKind.cpp`.
3. **Use case** — thêm một class trong `src/Application` có `Execute(paths)`. Nếu cần gì từ Windows, thêm phương thức vào một port (hoặc port mới), hiện thực ở `src/Infrastructure` và ở `tests/UnitTests/Fakes.h`.
4. **Nối dây** — thêm một nhánh trong `src/App/ActionRunner.cpp`.
5. **Menu** — thêm một class trong `src/ShellExtension/Commands.cpp` với CLSID mới, khai báo trong cả `packaging/AppxManifest.xml` lẫn `packaging/release/AppxManifest.xml`, và thêm vào `ShellExtensionActivationTests.cpp`.
6. **Phím tắt** (tùy chọn) — thêm vào `Settings::Defaults()` và `kAllActions` trong `Settings.cpp`, cùng `SettingsDialog.cpp` và dialog trong `App.rc`.
7. **File project** — file nguồn mới phải được thêm thủ công vào `.vcxproj` tương ứng.

Các use case có sẵn không phải sửa.

### Quy ước

- `/std:c++20 /W4 /WX /permissive- /utf-8`, static CRT, chỉ x64. Cấu hình chung nằm ở `Directory.Build.props` và `Directory.Build.targets`, không lặp trong từng project.
- Include theo đường dẫn từ `src`: `#include "Domain/Result.h"`.
- Trả lỗi bằng `et::domain::Result<T>`; không exception nào được vượt ranh giới COM.
- RAII cho mọi tài nguyên Windows/COM; không `new`/`delete` trần.
- Không bao giờ dùng `FOF_NOCONFIRMATION`, và không nới lỏng cổng chọn tab trong `ExplorerSelectionSource` — cả hai tồn tại để bảo vệ file của người dùng.
- Không đặt tên phương thức trùng macro của `windows.h` (`CreateDirectory`, `MoveFile`, `CopyFile`, `DeleteFile`…).
- Script phải chạy được trên Windows PowerShell 5.1 và chỉ dùng ký tự ASCII.
- Test chỉ được đụng tới file trong `%TEMP%\ExplorerMate.Tests`.

### Cần cộng đồng giúp: Windows trên ARM

Explorer Mate hiện chỉ được build và kiểm thử cho x64. Để hỗ trợ Windows trên ARM cần một người có thiết bị ARM, vì những phần quan trọng nhất không kiểm tra được bằng cách nào khác. Trong mã nguồn chưa thấy chỗ nào gắn riêng với x64, nên công việc dự kiến chủ yếu là cấu hình và kiểm thử:

- Thêm nền tảng ARM64 vào `ExplorerMate.sln`, các project và `Directory.Build.props`/`.targets` (CET, tức `/CETCOMPAT`, chỉ dành cho x64 nên phải bỏ), và thêm tham số nền tảng cho các script.
- Dựng gói MSIX cho ARM64 (`ProcessorArchitecture="arm64"` trong manifest).
- Thêm một job CI trên runner `windows-11-vs2026-arm` của GitHub để bản ARM64 được build và chạy test trực tiếp trên ARM.
- **Trên thiết bị ARM thật:** xác nhận ba lệnh hiện trong menu chuột phải và chạy đúng, hộp thoại mở lên phía trước, và phím tắt tác động đúng tab (`scripts\test-tab-detection.ps1` kiểm tra việc nhận diện tab và focus, vốn phụ thuộc cấu trúc cửa sổ của Explorer).

Nếu bạn giúp được, hãy mở issue hoặc pull request và cho biết thiết bị cùng bản Windows bạn đã thử.

### Đóng góp

Hoan nghênh issue và pull request. Trước khi mở pull request, hãy chạy `scripts\test.ps1` trên cả Debug và Release, thêm hoặc cập nhật test cho hành vi bạn thay đổi, và ghi trong mô tả những gì bạn đã thử bằng tay (menu và phím tắt không phủ hết được bằng test tự động).

## Ghi công

Ý tưởng, yêu cầu và việc kiểm thử thực tế là của **Tuan Nguyen** ([@anhtuank7c](https://github.com/anhtuank7c), [meohamhoc.vn](https://meohamhoc.vn)). Mã nguồn và tài liệu được viết bằng [Claude Code](https://claude.com/claude-code), AI coding agent của Anthropic, dựa trên những yêu cầu đó. Đây không phải mã viết tay, và dự án nói rõ điều ấy.

Cú pháp đổi tên hàng loạt lấy cảm hứng từ Total Commander; "New folder with selection" và "Duplicate" lấy cảm hứng từ Finder của macOS. Explorer Mate không liên kết với các sản phẩm đó, cũng không liên kết với Microsoft.

## Giấy phép

[MIT](LICENSE) © 2026 Tuan Nguyen.
