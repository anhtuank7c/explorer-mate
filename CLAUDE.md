# Explorer Mate (tên ngắn: ExMate)

Tiện ích C++ native thêm ba lệnh vào menu chuột phải của File Explorer trên Windows 11: gom vào thư mục mới, đổi tên hàng loạt (postfix có index), nhân bản tại chỗ. Kế hoạch đầy đủ: `windows11-explorer-tools-claude-plan.md`. Tiến độ: `docs/PROGRESS.md`.

## Lệnh

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Configuration All   # Debug + Release x64
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test.ps1  -Configuration Debug # layer check + build + test
```

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\install-dev.ps1    # đăng ký menu (cần Developer Mode), mặc định dùng bản Release
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\uninstall-dev.ps1  # gỡ đăng ký
```

`test.ps1 -NoBuild` bỏ qua bước build. CLSID của mỗi lệnh phải khớp ở ba nơi: class trong `src/ShellExtension`, `packaging/AppxManifest.xml`, và test activation. Output nằm ở `build\x64\<Configuration>\`.

## Kiến trúc

Phụ thuộc chỉ đi vào trong; `scripts\check-layers.ps1` kiểm tra bằng cách quét `#include`.

| Lớp | Thư mục | Namespace | Được include |
|---|---|---|---|
| Domain | `src/Domain` | `et::domain` | Chỉ C++ standard library |
| Application | `src/Application` | `et::app` | Domain |
| Infrastructure | `src/Infrastructure` | `et::infra` | Application, Domain, Windows SDK |
| Presentation | `src/App` (`ExMate.exe`), `src/ShellExtension` (`ExMate.Shell.dll`) | `et::ui` | Tất cả |

- Use case nằm ở Application, mỗi cái một class; phụ thuộc bên ngoài đi qua port (interface) khai báo ở Application và hiện thực ở Infrastructure/App.
- DLL menu không chạy nghiệp vụ: chỉ ghi request file rồi launch `ExMate.exe`.
- DLL menu chạy trong `DllHost.exe` (COM surrogate có package identity), không phải `explorer.exe`. Manifest tắt `FileSystemWriteVirtualization` để surrogate ghi thẳng vào `%LOCALAPPDATA%\ExMate`; đừng bỏ dòng đó.
- Thử một lệnh đã đăng ký mà không cần GUI: `& .\scripts\probe-command.ps1 -Clsid <clsid> -Path <item1>,<item2>`. Log của DLL: `%LOCALAPPDATA%\ExMate\logs\shell.log`.
- Thử dialog không cần người bấm: `& .\scripts\probe-dialog.ps1 -Action group|rename -Path <items> [-SetText @{<controlId>='text'}] [-Press ok]`.
- Sửa DLL/EXE xong phải chạy lại `install-dev.ps1` thì menu mới dùng bản mới.
- Không đặt tên phương thức trùng macro của `windows.h` (`CreateDirectory`, `MoveFile`, `CopyFile`, `DeleteFile`...).
- Không dùng `FOF_NOCONFIRMATION`. `FOF_SILENT` không chặn được hộp thoại xung đột của Shell, nên gateway phải tự kiểm tra đích trước.
- Shell trả đường dẫn dài; `%TEMP%` có thể là đường dẫn ngắn 8.3. So sánh đường dẫn phải qua `GetLongPathNameW`/`canonical`.

## Quy ước

- Cấu hình build chung nằm ở `Directory.Build.props` (thuộc tính) và `Directory.Build.targets` (cờ compiler/linker). Không lặp lại trong `.vcxproj`.
- `/std:c++20 /W4 /WX /permissive- /utf-8`, static CRT, chỉ x64. Toolset `v145`.
- Include theo đường dẫn từ `src`: `#include "Domain/Result.h"`.
- File nguồn mới phải được thêm thủ công vào `.vcxproj` tương ứng.
- Lỗi trả về bằng `et::domain::Result<T>`; không ném exception qua ranh giới COM.
- RAII cho mọi tài nguyên Windows/COM. Không `new`/`delete` trần.
- Script PowerShell phải chạy được trên Windows PowerShell 5.1 và chỉ dùng ký tự ASCII.
- Test dùng Microsoft C++ Unit Test Framework; test không được chạm file ngoài fixture trong `%TEMP%`.

- Agent phím tắt: `ExMate.exe --agent` / `--stop-agent`. `install-dev.ps1` tự dừng và chạy lại agent; build không bị chặn vì agent chạy từ `build\install`.
- Kiểm tra nhận diện tab/focus: `scripts\test-tab-detection.ps1` (tự mở và đóng một cửa sổ Explorer trên fixture). Xem trạng thái hiện tại: `ExMate.exe --diagnose-explorer | Out-String`.
- EXE là GUI-subsystem: trong PowerShell phải đưa vào pipeline (`| Out-String`, `| ForEach-Object`) thì mới chờ và lấy được output.
- `HotkeyMatcher` bỏ qua phím injected nên không thể test hook bằng `SendInput`/`SendKeys`.
- Callback của keyboard hook chỉ được làm việc rẻ (không COM, không I/O); việc nặng đi qua `PostMessage`.

## Giới hạn an toàn

- Không tự restart/kill Explorer, không tự sửa certificate trust store, không bật Developer Mode thay người dùng.
- Không nới lỏng cổng chọn tab trong `ExplorerSelectionSource` (không "lấy tab đầu tiên", không dùng selection cũ).
