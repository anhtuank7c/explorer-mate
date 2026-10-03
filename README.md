# Explorer Mate

**English** · [Tiếng Việt](#tiếng-việt)

A small companion for the Windows 11 File Explorer. It adds three commands that Explorer does not have, right where you already work: the right-click menu and the keyboard.

| Command | What it does | Default shortcut |
|---|---|---|
| **New folder with selection** | Creates a folder next to the selected items and moves them into it | Ctrl+Alt+N |
| **Bulk rename** | Renames many files at once with a name mask, a counter and a live preview | Ctrl+Alt+R |
| **Duplicate** | Copies the selected files or folders next to the originals | Ctrl+Alt+D |

Explorer Mate is not a file manager and does not replace Explorer. You keep Explorer, its tabs and everything you are used to; you just get a few more things to do with a selection.

> **Status: early development build.** It works on the author's machine (Windows 11, x64) and is not yet published. Installing currently requires Windows Developer Mode. Releases through the Microsoft Store, WinGet and GitHub are planned.

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
- Windows 11 x64 only. Windows 10 and ARM64 are not supported.
- Compared with Total Commander's rename tool, date/time placeholders, case conversion, regular expressions and saved presets are not there yet.

## Install (development build)

Requirements: Windows 11 x64, Visual Studio 18 (toolset v145) with "Desktop development with C++" and the Windows 11 SDK, and **Developer Mode** turned on (Settings → System → For developers).

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration All
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test.ps1 -Configuration Release
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\install-dev.ps1 -StartAgent
```

Uninstall with `scripts\uninstall-dev.ps1`. The tests only create and delete files under `%TEMP%\ExMate.Tests`.

`scripts\package-msix.ps1` builds the release-style MSIX package (unsigned unless you pass a certificate).

## Using it

Select files or folders in File Explorer and right-click; Windows may group the three commands under "Explorer Mate". With the tray agent running, the shortcuts in the table above work while the file list has the keyboard focus. Click the tray icon to change shortcuts, toggle "Start with Windows" or exit.

Running `ExMate.exe` with no arguments opens an introduction window. There is also a command line:

```powershell
ExMate.exe --action duplicate D:\Work\a.txt D:\Work\Assets
ExMate.exe --action group --name "Project A" D:\Work\a.txt D:\Work\b.jpg
ExMate.exe --action rename --mask "Trip_[C]" D:\Photos\IMG_7.jpg D:\Photos\IMG_12.jpg
```

If a command does nothing, check the logs in `%LOCALAPPDATA%\ExMate\logs`.

## How it is built

C++20, native Win32 and COM, no .NET or Electron. The code follows a layered (Clean Architecture) layout: pure rules in `src/Domain`, use cases and ports in `src/Application`, Windows adapters in `src/Infrastructure`, and the menu DLL and EXE on top. A script fails the test run if an inner layer includes Windows headers. The more detailed documents in `docs/` are written in Vietnamese:

- `docs/ARCHITECTURE.md` — layers, data-safety rules, design decisions
- `docs/TEST_MATRIX.md` — what has been tested, how, and what has not
- `docs/PROGRESS.md` — milestone log

## Credits

The idea, the requirements and the hands-on testing are by **Tuan Nguyen** ([@anhtuank7c](https://github.com/anhtuank7c), [meohamhoc.vn](https://meohamhoc.vn)). The code and documentation were written with [Claude Code](https://claude.com/claude-code), Anthropic's AI coding agent, working from those requirements. This is not a hand-written codebase, and it is presented as what it is.

The bulk rename syntax is inspired by Total Commander; "New folder with selection" and "Duplicate" by macOS Finder. Explorer Mate is not affiliated with either, nor with Microsoft.

## License

Not chosen yet. Until a license is added, the usual default applies: all rights reserved.

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

> **Trạng thái: bản phát triển, còn sớm.** Công cụ chạy được trên máy của tác giả (Windows 11, x64) và chưa phát hành chính thức. Hiện việc cài đặt cần bật Developer Mode của Windows. Kế hoạch là phát hành qua Microsoft Store, WinGet và GitHub.

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
- Chỉ hỗ trợ Windows 11 x64; không hỗ trợ Windows 10 và ARM64.
- So với công cụ đổi tên của Total Commander, hiện chưa có placeholder ngày giờ, đổi hoa/thường, biểu thức chính quy và lưu mẫu đặt tên.

## Cài đặt (bản phát triển)

Yêu cầu: Windows 11 x64, Visual Studio 18 (toolset v145) với workload "Desktop development with C++" và Windows 11 SDK, và đã bật **Developer Mode** (Settings → System → For developers).

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration All
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test.ps1 -Configuration Release
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\install-dev.ps1 -StartAgent
```

Gỡ bằng `scripts\uninstall-dev.ps1`. Test chỉ tạo và xóa file trong `%TEMP%\ExMate.Tests`.

`scripts\package-msix.ps1` dựng gói MSIX kiểu phát hành (chưa ký, trừ khi bạn truyền chứng thư).

## Cách dùng

Chọn file hoặc thư mục trong File Explorer rồi bấm chuột phải; Windows có thể gom ba lệnh vào mục "Explorer Mate". Khi agent ở khay đang chạy, các phím tắt trong bảng trên hoạt động lúc danh sách file đang giữ con trỏ bàn phím. Bấm icon ở khay để đổi phím tắt, bật/tắt "Start with Windows" hoặc thoát.

Chạy `ExMate.exe` không kèm tham số sẽ mở cửa sổ giới thiệu. Ngoài ra có dòng lệnh:

```powershell
ExMate.exe --action duplicate D:\Work\a.txt D:\Work\Assets
ExMate.exe --action group --name "Project A" D:\Work\a.txt D:\Work\b.jpg
ExMate.exe --action rename --mask "DaLat_[C]" D:\Anh\IMG_7.jpg D:\Anh\IMG_12.jpg
```

Nếu bấm lệnh mà không có gì xảy ra, xem log trong `%LOCALAPPDATA%\ExMate\logs`.

## Công cụ được xây dựng thế nào

C++20, Win32 và COM thuần, không dùng .NET hay Electron. Mã tổ chức theo lớp (Clean Architecture): luật thuần ở `src/Domain`, use case và port ở `src/Application`, adapter Windows ở `src/Infrastructure`, phía trên là DLL menu và EXE. Một script sẽ làm hỏng lượt test nếu lớp trong include header của Windows. Tài liệu chi tiết trong `docs/` viết bằng tiếng Việt:

- `docs/ARCHITECTURE.md` — các lớp, nguyên tắc an toàn dữ liệu, quyết định thiết kế
- `docs/TEST_MATRIX.md` — đã kiểm thử gì, bằng cách nào, và những gì chưa kiểm
- `docs/PROGRESS.md` — nhật ký theo milestone

## Ghi công

Ý tưởng, yêu cầu và việc kiểm thử thực tế là của **Tuan Nguyen** ([@anhtuank7c](https://github.com/anhtuank7c), [meohamhoc.vn](https://meohamhoc.vn)). Mã nguồn và tài liệu được viết bằng [Claude Code](https://claude.com/claude-code), AI coding agent của Anthropic, dựa trên những yêu cầu đó. Đây không phải mã viết tay, và dự án nói rõ điều ấy.

Cú pháp đổi tên hàng loạt lấy cảm hứng từ Total Commander; "New folder with selection" và "Duplicate" lấy cảm hứng từ Finder của macOS. Explorer Mate không liên kết với các sản phẩm đó, cũng không liên kết với Microsoft.

## Giấy phép

Chưa chọn. Cho tới khi có giấy phép, mặc định thông thường được áp dụng: giữ mọi quyền.
