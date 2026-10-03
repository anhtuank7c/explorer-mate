# Kiến trúc ExMate

## Luồng một lệnh

```text
File Explorer
   │  chuột phải → Invoke(IShellItemArray)
   ▼
DllHost.exe  ── ExMate.Shell.dll (src/ShellExtension)
   │  đọc đường dẫn, ghi %LOCALAPPDATA%\ExMate\requests\<guid>.etreq
   │  CreateProcess: ExMate.exe --request <file>
   ▼
ExMate.exe (src/App)
   │  đọc + xóa request, validate lại, mở dialog nếu cần
   ▼
Use case (src/Application) ──port──> adapter Windows (src/Infrastructure) ──> IFileOperation
```

DLL không hiện dialog, không đụng filesystem ngoài request file. Mọi nghiệp vụ nằm trong EXE, nên lỗi hay crash không ảnh hưởng Explorer.

## Các lớp

| Lớp | Project | Namespace | Phụ thuộc được phép |
|---|---|---|---|
| Domain | `src/Domain` | `et::domain` | C++ standard library |
| Application | `src/Application` | `et::app` | Domain |
| Infrastructure | `src/Infrastructure` | `et::infra` | Application, Domain, Windows SDK |
| Presentation | `src/App`, `src/ShellExtension` | `et::ui` | Tất cả |

`scripts\check-layers.ps1` quét `#include` và làm hỏng `test.ps1` nếu Domain/Application include header Windows hoặc lớp ngoài.

### Domain

| Thành phần | Vai trò |
|---|---|
| `Result<T>`, `Status`, `Error` | Trả lỗi không dùng exception |
| `PathText`, `ItemName` | Tách đường dẫn/tên; luật đặt tên Windows |
| `FolderName`, `Selection` | Value object đã validate |
| `INameCollation` | "Cùng tên" và "thứ tự Explorer" do nền tảng quyết định, nên được tiêm vào |
| `RenamePattern`, `RenamePlan` | Tính toàn bộ lô đổi tên trước khi mutate: đánh số, phát hiện trùng, sắp thứ tự bước, phá vòng bằng tên tạm |
| `OperationReport` | Kết quả theo từng item: Succeeded / Failed / Skipped / NotAttempted |

### Application

Use case: `GroupIntoNewFolderUseCase`, `BulkRenameUseCase`, `DuplicateInPlaceUseCase`. Mỗi cái nhận port qua constructor và có một phương thức `Execute(paths) → Result<OperationReport>`.

| Port | Trách nhiệm | Adapter thật | Adapter test |
|---|---|---|---|
| `IFileSystemProbe` | Hỏi filesystem (chỉ đọc) | `Win32FileSystemProbe` | `FakeFileSystem` |
| `IFileOperationGateway` | Thay đổi filesystem, không ghi đè | `ShellFileOperationGateway` | `FakeFileSystem` |
| `IUserPrompt` | Hỏi người dùng | `DialogPrompt`, `PresetPrompt` | `ScriptedPrompt` |
| `ILogger` | Ghi log | `FileLogger` | — |

Thêm một lệnh mới: một use case, một class trong `Commands.cpp`, một giá trị `ActionKind`, một nhánh trong `ActionRunner`, một CLSID trong manifest. Use case cũ không phải sửa.

## Bảo đảm an toàn dữ liệu

- **Không ghi đè, không merge.** Copy dùng `FOF_RENAMEONCOLLISION`. Move chỉ vào folder do chính request tạo (`CreateDirectoryW` thất bại nếu tên đã tồn tại). Đổi tên: kế hoạch từ chối trùng với mục ngoài selection, và gateway kiểm tra lại đích ngay trước khi giao cho Shell.
- **Không bao giờ dùng `FOF_NOCONFIRMATION`** — cờ này tự trả lời "có" cho câu hỏi ghi đè.
- **Không tự rollback.** `IFileOperation` không phải transaction; kết quả dở dang được báo theo từng item. Dọn dẹp duy nhất là `RemoveDirectoryW` trên folder vừa tạo, vốn chỉ xóa được folder rỗng.
- **Request không được tin.** EXE chỉ nhận file nằm trong thư mục `requests`, đuôi `.etreq`, giới hạn 64 MB, UTF-8 hợp lệ, đúng định dạng; sau đó use case validate lại selection với filesystem.
- **Không qua shell lệnh.** Selection đi qua file; command line chỉ chứa đường dẫn request có tên là GUID.

## COM

- DLL: WRL `RuntimeClass<ClassicCom, IExplorerCommand>`, `ThreadingModel=STA`, chạy trong surrogate `DllHost.exe`. `GetTitle`/`GetState` không chạm ổ đĩa ngoài thuộc tính của tối đa 256 item.
- EXE: một STA duy nhất (`ComApartment`, RAII) trên luồng chính; `IFileOperation::PerformOperations` tự bơm message.
- Không exception nào vượt ranh giới COM: `Invoke` và progress sink bọc `try/catch`.
- Tên/đích thực tế lấy từ `IFileOperationProgressSink::Post*Item`. Trả lỗi từ `PostRenameItem` hủy các thao tác còn lại — dùng để dừng chuỗi đổi tên ở lỗi đầu tiên.

## Phím tắt (Giai đoạn 2)

```text
bàn phím ──WH_KEYBOARD_LL──> Agent (ExMate.exe --agent, icon khay)
                               │ HotkeyMatcher: đúng chord? focus ở file list?
                               │ PostMessage cho chính nó (ra khỏi hook)
                               ▼
                             ExplorerSelectionSource: tab nào, selection gì
                               ▼
                             WorkerProcess: request file + ExMate.exe --request
```

Từ worker trở đi giống hệt đường menu. Agent không tự thao tác file.

- **Trong hook chỉ có việc rẻ:** `HotkeyMatcher` (thuần, đã unit test) và `FocusIsInFileList` (vài lời gọi user32). COM, `IShellWindows`, khởi động tiến trình đều chạy sau, trên message loop.
- **Xác định tab:** foreground là `CabinetWClass`; đúng một tab của frame đó đang hiện (cửa sổ `ShellTabWindowClass` đứng đầu); focus là `DirectUIHWND` con trực tiếp của `SHELLDLL_DefView` của chính tab đó. Lệch bất kỳ điểm nào thì từ chối — không bao giờ lấy "tab đầu tiên".
- **Fail-closed:** modifier không bao giờ bị nuốt; phím giả lập bị bỏ qua; AltGr (Right Alt) không tính là Alt; khi worker còn chạy thì không chặn phím nào; khóa/đổi session thì xóa trạng thái phím đang giữ.
- **Một agent mỗi session** (mutex `Local\ExMate.Agent`). Settings ở `%LOCALAPPDATA%\ExMate\settings.txt`; file hỏng thì chạy bằng mặc định và giữ nguyên file.
- **Điểm yếu đã biết:** cửa sổ agent nhận thông điệp nội bộ "đã bấm phím tắt" từ bất kỳ tiến trình nào cùng người dùng. Thứ bị tác động vẫn chỉ là selection trong tab đang focus.

## ADR-1: Đóng gói bằng sparse package, đăng ký qua Developer Mode

- **Bối cảnh:** menu chính của Windows 11 chỉ nhận `IExplorerCommand` từ ứng dụng có package identity.
- **Quyết định:** Win32 + package with external location; đăng ký bằng `Add-AppxPackage -Register AppxManifest.xml -ExternalLocation <dir>` khi Developer Mode bật, không ký.
- **Hệ quả:** không cần certificate cho máy dev. Phân phối cho máy khác sẽ cần package đã ký — chưa làm.
- **Phải giữ:** `desktop6:FileSystemWriteVirtualization=disabled` + capability `unvirtualizedResources`, nếu không DLL và EXE nhìn thấy hai thư mục `%LOCALAPPDATA%` khác nhau.

## ADR-2: Request file thay cho named pipe

- **Bối cảnh:** Giai đoạn 1 không có tiến trình thường trú.
- **Quyết định:** DLL ghi file `.etreq` rồi khởi động EXE; định dạng dòng văn bản thay vì JSON vì tên file Windows không chứa xuống dòng.
- **Hệ quả:** không cần IPC server, ACL pipe hay parser JSON. Giai đoạn 2 (hotkey) có thể thay `RequestFileStore` bằng pipe mà không sửa use case.

## ADR-3: Chế độ Silent không ghi Undo

- **Quyết định:** `OperationUi::Silent` (test, script) bỏ `FOF_ALLOWUNDO | FOFX_ADDUNDORECORD`; `Interactive` (menu) có.
- **Lý do:** thao tác của test không được chen vào Ctrl+Z của Explorer.
