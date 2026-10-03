# Tiến độ

## Giai đoạn 1

- [x] M0 — Khung dự án
- [x] M1 — Packaging spike
- [x] M2 — Domain + Application
- [x] M3 — Infrastructure + CLI
- [x] M4 — Dialog (logic đã kiểm bằng script; hiển thị chờ người dùng xác nhận)
- [x] M5 — Nối menu (đường COM đã kiểm bằng script; menu thật chờ người dùng xác nhận)
- [ ] M6 — Hoàn thiện và nghiệm thu (đã có script cài/gỡ, README, ARCHITECTURE, TEST_MATRIX; còn icon, chọn kết quả, đo Undo, kiểm thử thủ công)

## M0 — Khung dự án (03/10/2026)

Đã làm:

- `ExplorerMate.sln` với bảy project: Domain, Application, Infrastructure (static lib), App (`ExplorerMate.exe`), ShellExtension (`ExplorerMate.Shell.dll`), UnitTests, IntegrationTests.
- `Directory.Build.props` + `Directory.Build.targets` chứa cấu hình chung.
- `scripts/build.ps1`, `scripts/test.ps1`, `scripts/check-layers.ps1`.
- Mã khởi đầu: `Result<T>`/`Error`, `ActionKind`, `ComApartment`, `ExplorerMate.exe --version`, DLL với `DllGetClassObject` rỗng.
- `git init` (chưa commit).

Bằng chứng (Windows 11 Pro build 26300 x64, VS Community 18.10.3, MSVC 14.51.36231, toolset v145, SDK 10.0.26100.0):

- `scripts\build.ps1 -Configuration All`: Debug|x64 và Release|x64 build sạch, không warning.
- Dòng lệnh compiler thực tế có `/std:c++20 /W4 /WX /MT /permissive- /utf-8`.
- `scripts\test.ps1` trên Debug và Release: layer check pass; 6/6 test pass (5 unit, 1 integration).
- `ExplorerMate.exe --version` in `ExplorerMate 0.1.0`, exit 0; tham số lạ exit 2.

Giới hạn:

- DLL chưa đăng ký lệnh nào; EXE chưa có action nào.
- `check-layers.ps1` chỉ quét `#include` trực tiếp.

## M1 — Packaging spike (03/10/2026)

Đã làm:

- `ExplorerCommandBase` (WRL, `IExplorerCommand`) và lệnh thử `DumpSelectionCommand` ghi selection + tiến trình host vào `%LOCALAPPDATA%\ExplorerMate\logs\shell.log`.
- Port `ILogger`; adapter `FileLogger`, `ProductDataDirectory`, `ReadFileSystemPaths`, `ToUtf8`.
- `packaging/AppxManifest.xml` (sparse package `ExplorerMate.Dev`, đăng ký cho `*` và `Directory`) và ba logo tạm.
- `scripts/install-dev.ps1`, `scripts/uninstall-dev.ps1`: binary được stage sang `build\install` để DLL đang load không chặn lần build sau.
- ADR: đăng ký bằng Developer Mode + `Add-AppxPackage -Register ... -ExternalLocation`, không ký (người dùng chọn 03/10/2026).

Bằng chứng:

- Debug và Release build sạch; 9/9 test pass trên cả hai (thêm `FileLoggerTests`, `ShellExtensionActivationTests` gọi `DllGetClassObject` trực tiếp).
- `install-dev.ps1` đăng ký `ExplorerMate.Dev_0.1.0.0_x64__drj9t9cmp0jea` (`IsDevelopmentMode=True`, `SignatureKind=None`, `Status=Ok`).
- `CoCreateInstance` CLSID từ tiến trình ngoài package thành công.
- `uninstall-dev.ps1` gỡ package và xóa `build\install`; chạy lại install thành công (idempotent).

Kiểm thử thủ công trong Explorer (người dùng, build 26300):

- PASS — lệnh "ExplorerMate: log selection" hiện ở menu chuột phải chính.
- PASS — bấm lệnh với 1 file: log ghi đúng đường dẫn của file đã chọn.
- NOT RUN — nhiều file/folder/mixed bấm bằng tay trong Explorer. Đường này mới được kiểm qua `scripts\probe-command.ps1` (kích hoạt COM như Explorer): 3 mục gồm folder, file và tên tiếng Việt `ảnh đẹp.jpg` đều đúng.

Phát hiện:

- DLL chạy trong `C:\WINDOWS\system32\DllHost.exe` (COM surrogate có package identity), **không** nằm trong `explorer.exe`. Lỗi trong DLL không kéo sập Explorer; build lại không cần restart Explorer, chỉ cần surrogate thoát.
- Mặc định Windows chuyển hướng ghi `%LOCALAPPDATA%` của surrogate sang `%LOCALAPPDATA%\Packages\ExplorerMate.Dev_drj9t9cmp0jea\LocalCache\Local\...`. Đã tắt bằng `desktop6:FileSystemWriteVirtualization=disabled` trong manifest; sau đó log nằm đúng ở `%LOCALAPPDATA%\ExplorerMate\logs\shell.log`. Điều này bắt buộc cho request file ở M5 vì DLL và EXE phải thấy cùng một thư mục.

Trạng thái: M1 đạt. Package `ExplorerMate.Dev` đang được cài.

## M2 — Domain + Application (03/10/2026)

Đã làm:

- Domain: `PathText`, `ItemName` (tách stem/extension, luật tên Windows), `FolderName`, `Selection`, `INameCollation` + `SimpleNameCollation`, `RenamePattern`/`RenamePlan` (đánh số, phát hiện trùng, sắp thứ tự bước, phá vòng bằng tên tạm), `OperationReport`.
- Application: port `IFileSystemProbe`, `IFileOperationGateway`, `IUserPrompt`; `ValidateSelection`; ba use case `GroupIntoNewFolderUseCase`, `BulkRenameUseCase`, `DuplicateInPlaceUseCase`.
- Test: `FakeFileSystem` (in-memory, hiện thực cả hai port filesystem) và `ScriptedPrompt`.

Bằng chứng: 56/56 test pass trên Debug và Release; layer check pass (Domain/Application không include header Windows).

Quyết định thiết kế:

- So sánh tên và thứ tự "kiểu Explorer" phụ thuộc nền tảng nên được tiêm qua `INameCollation`; bản Windows chính xác sẽ nằm ở Infrastructure (M3).
- Use case không tin prompt: tên folder do prompt trả về được validate lại trước khi tạo.
- Hủy ở dialog trả về `ErrorCode::Cancelled`, lớp ngoài coi đó là im lặng chứ không phải lỗi.

Giới hạn: vòng đổi tên chỉ xảy ra được với collation đảo thứ tự (test dùng `ReversedCollation`); với thứ tự tự nhiên thì chỉ có chuỗi phụ thuộc.

## M3 — Infrastructure + CLI (03/10/2026)

Đã làm:

- `ShellFileOperationGateway` (`IFileOperation` + progress sink), `Win32FileSystemProbe`, `WindowsNameCollation`, `ErrorText`, `RequestFileStore`.
- `ActionRequest` (định dạng request dạng dòng văn bản UTF-8, đuôi `.etreq`) ở Application.
- `ExplorerMate.exe`: `--action <group|rename|duplicate> <item>...` hoặc `--request <file>`; `--silent`, `--name`, `--stem`, `--separator`, `--start`, `--digits`. Exit code: 0 xong hết, 1 lỗi/dở dang, 2 sai tham số, 3 hủy.

Bằng chứng:

- 78/78 test pass trên Debug và Release (22 integration test chạy thao tác thật trong `%TEMP%\ExplorerMate.Tests`).
- Chạy CLI bản Release trên fixture: duplicate file + folder (`IMG_7 - Copy.jpg`, `Assets - Copy\logo.png`), rename với tên chung (`IMG_7.jpg`,`IMG_12.jpg` → `DaLat_01.jpg`,`DaLat_02.jpg`, đúng thứ tự tự nhiên), rename giữ tên gốc (`ảnh.jpg` → `ảnh_01.jpg`), group (`Project A\{DaLat_01.jpg, Assets\logo.png}`), group không có tên → exit 3, rename có folder → bị từ chối, exit 1.

Hành vi Shell đã đo được:

- Tên bản sao do Shell đặt là `<tên> - Copy<đuôi>` trên máy này; lấy từ `PostCopyItem`.
- Move/rename thành công trả mã `COPYENGINE_S_DONT_PROCESS_CHILDREN`, không phải `S_OK`; chỉ `COPYENGINE_S_USER_IGNORED` mới là bị bỏ qua.
- Shell trả đường dẫn dài; `%TEMP%` trên máy này là đường dẫn ngắn 8.3 nên gateway chuẩn hóa bằng `GetLongPathNameW` trước khi đối chiếu.
- **`FOF_SILENT | FOF_NOERRORUI` không chặn được hộp thoại "Replace or Skip"** khi đích đổi tên đã tồn tại. Vì vậy gateway tự kiểm tra đích trước khi giao cho Shell và trả `Failed` ngay; không bao giờ dùng `FOF_NOCONFIRMATION`.
- Trả lỗi từ `PostRenameItem` hủy các bước còn lại, dùng để dừng chuỗi đổi tên ở lỗi đầu tiên (đã test với file bị khóa).
- Chế độ Silent không ghi undo record để test không làm bẩn Ctrl+Z của Explorer. Hành vi Undo ở chế độ Interactive: NOT RUN.

Khác với kế hoạch: request file là văn bản theo dòng thay vì JSON (tên file trên Windows không chứa được xuống dòng, nên không cần parser JSON).

## M5 — Nối menu (03/10/2026, làm trước M4)

Đã làm:

- Ba lệnh thật thay cho lệnh thử: `GroupIntoNewFolderCommand`, `BulkRenameCommand`, `DuplicateInPlaceCommand` trong `src/ShellExtension/Commands.cpp`; manifest đăng ký ba CLSID.
- `WorkerLauncher`: ghi request file rồi `CreateProcessW` `ExplorerMate.exe --request <file>` (đường dẫn tuyệt đối, cạnh DLL).
- `GetState` ẩn Bulk rename khi selection có folder (ZIP không bị coi là folder).

Bằng chứng (qua `scripts\probe-command.ps1`, tức kích hoạt COM như Explorer, package đã đăng ký):

- Duplicate với 1 file + 1 folder → tạo `a - Copy.txt` và `Assets - Copy\logo.png`; request file được dọn (0 file tồn).
- Bulk rename: `GetState` trả `ECS_HIDDEN` khi chọn folder, `ECS_ENABLED` khi chọn file.
- 79/79 test pass (test activation kiểm tra tiêu đề của cả ba CLSID).

## M4 — Dialog (03/10/2026)

Đã làm:

- `DialogPrompt`: hộp "New folder with selection" (validate từng phím, OK bị khóa khi tên không hợp lệ) và hộp "Bulk rename" (tên chung, separator, số bắt đầu, list preview hai cột cập nhật trực tiếp).
- `ReportPresenter`: message box liệt kê mục lỗi/bị bỏ qua khi kết quả không trọn vẹn; hủy thì im lặng.
- Manifest EXE: Common Controls v6, DPI PerMonitorV2, long path aware. Dialog định nghĩa trong `App.rc`.
- CLI: có `--name`/`--stem`/... hoặc `--silent` thì không mở dialog.

Bằng chứng (qua `scripts\probe-dialog.ps1`, điều khiển dialog thật từ ngoài tiến trình):

- Group: gợi ý `New Folder`, OK bật; gõ `assets` (đã tồn tại) → báo trùng, OK tắt; gõ `a:b` → báo ký tự cấm, OK tắt; gõ `Project A` + OK → tạo folder và move `notes.txt`, `Assets` vào, exit 0; Cancel → exit 3, không thay đổi.
- Rename: mặc định separator `_`, start `1`, preview 2 dòng; separator `?` → báo lỗi tên, OK tắt; tên chung `DaLat` + OK → `DaLat_01.jpg`, `DaLat_02.jpg`, exit 0.

NOT RUN (cần người dùng thao tác thật trong Explorer):

- Ba lệnh hiện trong menu chuột phải; Bulk rename ẩn khi có folder.
- Dialog mở lên phía trước cửa sổ Explorer và nhận bàn phím ngay (probe chạy từ script nền nên không đo được điều này).
- Hiển thị dialog ở DPI khác 100%, tab order, phím Enter/Escape.
- Progress/Cancel với file lớn; Ctrl+Z sau mỗi lệnh.

## Phản hồi của người dùng sau khi thử (03/10/2026)

- Duplicate và New folder with selection: hoạt động đúng kỳ vọng trong Explorer.
- Bulk rename: yêu cầu làm lại theo kiểu Multi-Rename Tool của Total Commander; mặc định thêm gạch dưới và index theo thứ tự.

Đã làm lại Bulk rename:

- Domain: `RenameMask` (placeholder `[N]`, `[E]`, `[C]`, `[P]`, dải ký tự, ngoặc nguyên văn); `RenamePattern` giờ gồm mask tên, mask đuôi, bộ đếm (start/step/digits), tìm–thay thế. Mặc định `[N]_[C]` + `[E]`.
- Dialog mới: hai ô mask, nút chèn placeholder, nhóm Counter, nhóm Search and replace, preview.
- CLI: `--mask`, `--ext-mask`, `--start`, `--step`, `--digits`, `--search`, `--replace` (bỏ `--stem`, `--separator`).
- Use case, gateway, port không đổi; chỉ `BuildRenamePlan` nhận thêm tên thư mục cha cho `[P]`.

Bằng chứng: 89/89 test pass (Debug + Release). Qua `probe-dialog.ps1`: mặc định cho `IMG_7_01.jpg`, `IMG_12_02.jpg`, `IMG_100_03.jpg`; mask `[P] [C]` start 10 step 5 digits 3 với thay `Trip`→`DaLat` cho `DaLat 010.jpg`, `DaLat 015.jpg`, `DaLat 020.jpg`; mask sai, ô số trống, mask không có bộ đếm đều khóa OK kèm thông báo; nút chèn `[P]`, `[C]` hoạt động. Package đã cài lại bằng bản này.

Chưa làm so với Total Commander: placeholder ngày giờ, đổi hoa/thường, vị trí đếm từ cuối, regex, sắp xếp thủ công, lưu mẫu.

Người dùng xác nhận sau đó: "bulk rename work great".

## Giai đoạn 2

- [x] M7 — Spike nhận diện tab (gate đạt)
- [x] M8 — Agent + hotkey + Settings (người dùng xác nhận 03/10/2026 sau khi thử phím thật: "it all works"; không ghi chi tiết từng tình huống)

## M7 — Spike nhận diện tab (03/10/2026)

Đã làm:

- `infra::EnumerateExplorerTabs` (qua `IShellWindows`): mỗi tab có frame, cửa sổ tab (`ShellTabWindowClass`), cửa sổ view (`SHELLDLL_DefView`), thư mục, selection. Tab đang hiện = cửa sổ tab đứng đầu trong các con `ShellTabWindowClass` của frame (`FindWindowEx`).
- `infra::ExplorerSelectionSource` (port `ISelectionSource`): chỉ trả selection khi foreground là frame Explorer, đúng một tab đang hiện, và focus bàn phím là `DirectUIHWND` con trực tiếp của view của chính tab đó.
- `ExplorerMate.exe --diagnose-explorer [--delay n | --watch n]`.
- `scripts\test-tab-detection.ps1`: tự mở một cửa sổ Explorer trên fixture, thêm tab bằng UI Automation, đặt selection khác nhau, chuyển từng tab và đối chiếu.

Bằng chứng (build 26300):

- Ba tab trong một frame — FolderA `[a1]`, FolderB `[b1,b2]`, FolderA `[a2,a3]` (hai tab cùng thư mục, selection khác nhau): mỗi lần chuyển tab, đúng một tab được nhận là active với đúng thư mục và selection. Chạy cùng lúc với một cửa sổ Explorer khác của người dùng.
- Focus ở file list: chuỗi cửa sổ `DirectUIHWND < SHELLDLL_DefView < ... < ShellTabWindowClass < CabinetWClass`; đích của phím tắt = `[a2.txt][a3.txt]`.
- Đang đổi tên (F2): focus là `Edit < CtrlNotifySink < DirectUIHWND < SHELLDLL_DefView ...`; phím tắt bị từ chối.
- `IFolderView2::GetSelection` trả lỗi khi không chọn gì, nên đếm bằng `ItemCount(SVGIO_SELECTION)` trước.

NOT RUN: focus ở thanh địa chỉ, ô tìm kiếm, khung điều hướng (theo thiết kế bị từ chối vì không phải `DirectUIHWND` con của view, nhưng chưa đo); đổi tab đúng lúc bấm phím.

## M8 — Agent + hotkey + Settings (03/10/2026)

Đã làm:

- Domain: `KeyChord` (parse/format, bắt buộc có Ctrl/Alt/Win; phím là chữ, số, F1–F24).
- Application: `Settings` (bật/tắt + một chord mỗi action, định dạng văn bản, từ chối trùng chord), `HotkeyMatcher` (máy trạng thái thuần cho hook: không nuốt modifier, khớp đúng modifier, AltGr không tính là Alt, chặn auto-repeat, nuốt cả key-up, bỏ qua phím injected).
- Infrastructure: `KeyboardHook` (`WH_KEYBOARD_LL`), `SettingsFile`, `Autostart` (khóa Run của HKCU), `WorkerProcess` (dùng chung cho DLL menu và agent).
- App: `Agent` (`--agent`): single instance, icon khay với menu Bật/tắt phím tắt, Settings, Start with Windows, Exit; `--stop-agent`; `SettingsDialog` với ô hotkey chuẩn của Windows.
- Mặc định: Ctrl+Alt+N (gom), Ctrl+Alt+R (đổi tên), Ctrl+Alt+D (nhân bản). Autostart mặc định tắt.
- Khi worker (và dialog của nó) còn chạy, agent không chặn phím nào.
- `install-dev.ps1` dừng agent trước khi thay file và khởi động lại; `-StartAgent` để bật. `uninstall-dev.ps1` dừng agent và gỡ mục autostart nếu trỏ vào bản dev.

Bằng chứng:

- 107/107 test pass (Debug + Release); 18 test mới cho chord, settings, matcher.
- Vòng đời agent: khởi động, agent thứ hai tự thoát, `--stop-agent` dừng êm (exit 0), log ghi "Agent started/stopped".

NOT RUN:

- **Bấm phím thật.** Agent bỏ qua phím do phần mềm giả lập (theo thiết kế), nên chỉ người dùng mới thử được hook.
- `scripts\test-agent-hotkey.ps1` (gửi thẳng thông điệp "đã bấm Duplicate" cho agent khi file list đang focus): ba lần chạy đều SKIPPED vì script không đưa được cửa sổ thử lên foreground.
- Dialog Settings, menu khay, Start with Windows, dialog của worker có lên trước Explorer khi gọi bằng phím tắt không.

## Cửa sổ giới thiệu (03/10/2026)

- Chạy EXE không tham số hoặc `--about`, hoặc mục "About..." ở menu khay, mở dialog giới thiệu: tên + phiên bản, mô tả, tác giả, liên kết `https://meohamhoc.vn` (SysLink, mở bằng trình duyệt mặc định).
- Tác giả hiện là chỗ giữ chỗ `(your name here)` theo lựa chọn của người dùng; sửa ở `src\Domain\ProductInfo.cpp`.
- Đã kiểm bằng script: cửa sổ mở, đọc được tiêu đề `ExplorerMate 0.1.0`, dòng tác giả và liên kết; đóng trả exit 0. NOT RUN: bấm liên kết mở trình duyệt, bố cục nhìn bằng mắt, mục About ở khay.
- Sửa `install-dev.ps1`: gỡ package tự đóng agent, nên phải hỏi/dừng agent trước khi gỡ thì mới biết để chạy lại. Đã kiểm: cài lại không kèm `-StartAgent` vẫn đưa agent trở lại.

## Đổi tên dự án (03/10/2026)

- Người dùng chọn tên hiển thị **Explorer Mate**, tên ngắn **ExplorerMate** (trước đó: ExplorerTools).
- Đã đổi: `ExplorerMate.sln`, `ExplorerMate.exe`, `ExplorerMate.Shell.dll`, package `ExplorerMate.Dev`, header request/settings, thư mục dữ liệu `%LOCALAPPDATA%\ExplorerMate`, giá trị autostart, class cửa sổ agent, script, tài liệu. `ProductName()` = tên hiển thị, `ProductShortName()` = tên ngắn. Namespace C++ vẫn là `et::`. Tên file kế hoạch giữ nguyên.
- Mọi chỗ ghi "ExplorerMate" trong các mục lịch sử phía trên vốn là "ExplorerTools" tại thời điểm đó; riêng tên package cũ là `ExplorerTools.Dev_..._drj9t9cmp0jea`.
- Bằng chứng: gỡ package cũ, build sạch từ đầu, 107/107 test pass (Debug + Release), cài package mới `ExplorerMate.Dev_0.1.0.0_x64__6ze16rkz1ps82`, lệnh Duplicate qua COM tạo `a - Copy.txt`, agent chạy, `--version` in `Explorer Mate 0.1.0`.
- Còn lại của tên cũ: thư mục log `%LOCALAPPDATA%\ExplorerTools` (không có settings để chuyển; chưa xóa).

## Đổi tên lần hai: ExMate → ExplorerMate (03/10/2026)

- Người dùng muốn mọi nơi thống nhất là Explorer Mate. Tên hiển thị giữ "Explorer Mate"; định danh kỹ thuật (không chứa được dấu cách hoặc bất tiện khi có) đổi từ tên ngắn cũ sang `ExplorerMate`: `ExplorerMate.exe`, `ExplorerMate.Shell.dll`, `ExplorerMate.sln`, package `ExplorerMate` / `ExplorerMate.Dev`, `%LOCALAPPDATA%\ExplorerMate`, header request/settings, class cửa sổ agent, mutex.
- Các mục lịch sử phía trên ghi `ExplorerMate` ở những chỗ mà tại thời điểm đó tên là ExMate hoặc ExplorerTools (do thay thế hàng loạt).
- Thêm version resource (FileDescription/ProductName = "Explorer Mate") nhưng chỉ nhúng khi build với `-EmbedVersionInfo`.
- **Smart App Control** (đang bật trên máy dev) bắt đầu chặn một số file EXE chưa ký vừa build: 4 trong 9 file EXE build ra trong phiên này bị chặn, có và không có version resource, cả Debug lẫn Release; cùng mã nguồn build lại thì có bản qua có bản không. Phán quyết gắn với từng file (copy sang chỗ khác vẫn giữ nguyên). Chưa rõ có liên quan tới tên mới hay không; trước khi đổi tên chưa lần nào bị chặn. Ký bằng chứng thư được tin cậy là cách giải quyết thật sự.
- Script cài/gỡ giờ bỏ qua lỗi khi EXE cũ bị chặn lúc gọi `--stop-agent`.
- Bằng chứng: 107/107 test pass (Debug + Release); package loose `ExplorerMate_0.1.0.0_x64__jn0ge0denfdd8` đăng ký; Duplicate qua COM tạo `a - Copy.txt`; agent chạy với tên tiến trình `ExplorerMate.exe`; dữ liệu ở `%LOCALAPPDATA%\ExplorerMate`.
- Còn lại của tên cũ trên máy: không còn gì: hai thư mục log cũ trong `%LOCALAPPDATA%` đã được xóa theo yêu cầu người dùng.

## Review cấu trúc, môi trường, bảo mật và quy trình phát hành (03/10/2026)

Nguồn phát hiện: một lượt review bảo mật độc lập (không phải người viết mã), MSVC `/analyze`, BinSkim, và tự rà cấu trúc/môi trường/quy trình. Không có lỗi mức nghiêm trọng hay cao.

Đã sửa:

- **Phím tắt nhầm sau UAC (trung bình):** trạng thái modifier theo dõi từ event có thể "kẹt" khi key-up xảy ra sau hộp thoại UAC/cửa sổ elevated; giờ chord khớp phải được đối chiếu với trạng thái bàn phím thật (`PhysicalModifiersMatch`), lệch thì bỏ qua và reset.
- **Mật khẩu chứng thư trên dòng lệnh (trung bình):** `package-msix.ps1` ký bằng thumbprint từ certificate store, có timestamp; không còn tham số mật khẩu.
- Thông điệp nội bộ của agent không còn mang action; handler kiểm tra lại các điều kiện và có cờ chống re-entrancy. Xóa `test-agent-hotkey.ps1` vì nó dựa vào lỗ hổng này.
- Đường dẫn nguồn bị từ chối nếu có `.`/`..`, dấu chấm/khoảng trắng cuối, ký tự điều khiển, `:`; thêm tên thiết bị reserved còn thiếu.
- Request file chỉ nhận UTF-8 chính xác (surrogate lẻ bị từ chối); giới hạn 5.000 item.
- Worker báo lỗi khi request bị từ chối trên đường menu (trước đây im lặng); `wWinMain` có catch-all; dọn request cũ không ném exception.
- Lô đổi tên dừng ở bất kỳ bước nào không hoàn tất (kể cả người dùng chọn Skip).
- Log: mỗi mục một dòng, xoay vòng ở 1 MB; `--watch` không còn ghi tên file ra log; settings file giới hạn 64 KB.
- Đóng gói từ thư mục staging sạch; xóa thư mục qua `Remove-BuildFolder` (chỉ trong `build\`, từ chối nếu có junction).
- Build: Control Flow Guard, CET, `/DEPENDENTLOADFLAG:0x800`; 5 cảnh báo `/analyze` đã sửa.
- Một nguồn phiên bản duy nhất `src/Domain/Version.h` cho mã, resource, manifest, script.
- Thêm `.gitattributes`, `.editorconfig`, `.vsconfig`, `SECURITY.md`, `CHANGELOG.md`, mẫu pull request, Dependabot, `docs/RELEASING.md`.
- CI (`ci.yml`, `codeql.yml`) và quy trình phát hành (`release.yml`).

Bằng chứng: 113/113 test pass (máy dev và CI, Debug + Release); CI xanh ngay lần đầu; CodeQL 0 cảnh báo; BinSkim 0 lỗi; chạy thử `release.yml` thủ công tạo được gói MSIX, checksum và attestation, không tạo release. Bản cài trên máy dev đã cập nhật bằng bản đã sửa; lệnh Duplicate qua COM vẫn chạy.

Chưa làm / để lại:

- Phím thật sau khi sửa agent: chưa ai bấm thử lại.
- BA2024 (Spectre) cần cài thêm thư viện spectre-mitigated; BA2027 (SourceLink): chấp nhận.
- So khớp O(n²) trong `Selection`/`RenamePlan` vẫn còn, chỉ chặn bằng giới hạn 5.000 item; preview chưa debounce.
- File tạm `~explorermate-N.tmp` khi lô đổi tên có vòng bị lỗi giữa chừng: báo cáo chưa nêu tên gốc.
- Junction bên trong thư mục được duplicate vẫn do Shell xử lý.
- Cài đặt repo cần chủ repo bật: private vulnerability reporting (để `SECURITY.md` đúng), branch protection cho `main`, Dependabot alerts.
- Chưa có chứng thư ký; bước ký trong `release.yml` chưa tồn tại.

## Icon ứng dụng (03/10/2026)

- Thiết kế do người dùng chọn trên canvas Claude Design: thư mục vàng của Explorer với hai sparkle xanh dương, viền tách màu trắng (phương án "A1 - Folder sparkle"). Người dùng chọn xanh dương vì hợp mệnh Mộc (Kỷ Tỵ 1989) theo quan niệm ngũ hành.
- Hai file gốc `packaging/icon/ExplorerMate.svg` và `ExplorerMate-small.svg` (bản rút gọn cho 16-24 px: một sparkle to, không có sparkle nhỏ).
- `scripts/build-icons.ps1` render SVG bằng Edge headless ra: `src/App/ExplorerMate.ico` (16-256 px), 21 logo trong `packaging/Assets` (gồm các biến thể targetsize/unplated), `packaging/store/StoreLogo-300.png`.
- EXE có icon (`IDI_APP`); khay hệ thống, thanh tiêu đề các dialog và cửa sổ giới thiệu dùng icon này. Gói có `resources.pri` (makepri) để Windows chọn đúng biến thể logo.
- Bằng chứng: 113/113 test pass; gói MSIX pack được; chụp cửa sổ giới thiệu của bản đã cài thấy icon ở thanh tiêu đề và trong dialog; bảng so cỡ 16/24/32/48 px trên nền sáng và tối đọc được.
- Người dùng xác nhận (03/10/2026): icon ở taskbar, menu và Start menu đều hiển thị đẹp.

### Icon cho ba lệnh menu

- Người dùng chọn dùng icon của Lucide (giấy phép ISC, ghi trong `THIRD_PARTY_NOTICES.md`): `folder-plus` (gom nhóm), `pen-line` (đổi tên), `copy` (nhân bản). File gốc ở `packaging/icon/menu/`.
- Menu chuột phải không tự đổi màu icon, nên mỗi icon có hai bản: nét tối cho menu sáng, nét trắng cho menu tối. `IExplorerCommand::GetIcon` trả `"<đường dẫn DLL>,-<id>"` và chọn bản theo `AppsUseLightTheme` trong registry.
- `build-icons.ps1` sinh sáu file `src/ShellExtension/icons/*.ico` (16, 20, 24, 32 px).
- Bằng chứng: 114/114 test pass (thêm test kiểm tra icon mỗi lệnh trỏ tới thực sự có trong DLL); gọi `GetIcon` qua COM trên bản đã cài trả về id 101/103/105 (máy đang ở giao diện sáng); bảng so cỡ 16 px trên nền sáng và tối đọc được.
- NOT RUN: nhìn icon trong menu chuột phải thật, và ở giao diện tối.

## Lỗi: Explorer không tự cập nhật sau khi chạy lệnh (03/10/2026)

- Người dùng báo: sau "New folder with selection" phải bấm F5 mới thấy kết quả.
- Tái hiện bằng `scripts\test-view-refresh.ps1` (mở một cửa sổ Explorer trên fixture, chạy lệnh qua worker, so số mục cửa sổ đang hiển thị với trên đĩa): 3/3 lần cửa sổ hiện 4 mục thay vì 2 — thư mục mới có hiện, nhưng hai file đã chuyển đi vẫn còn trong danh sách.
- Nguyên nhân (suy ra từ kết quả đo, khớp với cách `SHChangeNotify` hoạt động): thông báo thay đổi của Shell được xếp hàng trong tiến trình gửi; worker thoát ngay sau thao tác nên thông báo không tới Explorer.
- Sửa: `ShellFileOperationGateway` gửi `SHChangeNotify` kèm `SHCNF_FLUSH` sau mỗi thao tác (`SHCNE_MKDIR`/`SHCNE_RMDIR` cho thư mục tạo/xóa, `SHCNE_UPDATEDIR` cho thư mục nguồn và đích).
- Sau khi sửa: 3/3 lần cửa sổ hiện đúng 2 mục, không cần refresh. 114/114 test pass. Bản cài trên máy dev đã cập nhật.
- `--diagnose-explorer` giờ in thêm `itemsInView` cho mỗi tab.
- NOT RUN: kiểm tra tương tự cho Duplicate và Bulk rename (cùng cơ chế thông báo, chưa đo riêng); người dùng chưa xác nhận lại trên thư mục thật.

## Giai đoạn 3 — Phát hành

Kênh: Microsoft Store, WinGet, file tải trực tiếp từ GitHub. Cả ba dùng chung một gói MSIX đầy đủ; bản dev vẫn dùng sparse package.

| Kênh | Chữ ký | Điều kiện |
|---|---|---|
| Store | Store ký | Tài khoản Partner Center; đặt chỗ tên; Identity Name/Publisher lấy từ Partner Center |
| WinGet | — | Có ngay qua nguồn `msstore` khi đã lên Store; manifest trong `winget-pkgs` cần bản GitHub đã ký |
| GitHub | Chứng thư của tác giả | Gói do Store ký không phát hành lại được; cần chứng thư riêng (SignPath Foundation/Certum cho mã nguồn mở) |

- [x] R1 — Gói MSIX đầy đủ
- [ ] R2 — Sửa mã cho bản đóng gói: autostart qua startup task của package thay cho khóa Run; mở từ Start menu thì bật agent; kiểm tra thư mục dữ liệu nhất quán giữa DLL, worker và agent khi cài từ `.msix` thật
- [ ] R3 — Nội dung phát hành: icon thật, số phiên bản, chính sách quyền riêng tư, mô tả và ảnh chụp cho Store (tác giả và giấy phép MIT đã xong)
- [ ] R4 — Thử cài `.msix` đã ký trên máy không bật Developer Mode
- [ ] R5 — Nộp Store, rồi xác nhận `winget install` từ nguồn `msstore`
- [ ] R6 — GitHub Releases bằng chứng thư riêng, CI build + ký; nộp manifest vào `winget-pkgs`

File kế hoạch gốc đã bị xóa khỏi repo và khỏi lịch sử git theo yêu cầu người dùng (03/10/2026); các mục "file plan" nhắc ở phần lịch sử phía trên không còn tra cứu được.

### R1 — Gói MSIX đầy đủ (03/10/2026)

- `packaging\release\AppxManifest.xml` (full package, chỉ quyền `runFullTrust`, có mục Start menu) và `scripts\package-msix.ps1` (layout → `makeappx pack` → ký tùy chọn; `-RegisterLoose` để thử không cần chứng thư).
- Bằng chứng: pack ra `build\package\ExplorerMate_0.1.0.0_x64.msix` (379 KB), qua kiểm tra manifest của `makeappx`. Đăng ký loose `ExplorerMate_0.1.0.0_x64__6ze16rkz1ps82`: lệnh Duplicate qua COM tạo `a - Copy.txt`, request file được dọn; có mục Start menu `ExplorerMate_6ze16rkz1ps82!ExplorerMate`; agent chạy được từ thư mục layout.
- Trạng thái máy: bản dev `ExplorerMate.Dev` đã gỡ, bản full loose đang đăng ký từ `build\package\layout`, agent chạy từ đó. Hai script cài chặn nhau để không đăng ký trùng lệnh menu.
- NOT RUN: cài từ file `.msix` đã ký (cần chứng thư được tin cậy); hành vi ảo hóa `%LOCALAPPDATA%` khi cài thật có thể khác bản loose; menu thật trong Explorer và phím tắt với bản full.

## Bước tiếp theo

- Người dùng cung cấp nội dung dòng tác giả.
- R2: startup task, bật agent khi mở từ Start menu.
- M6: icon cho lệnh, chọn kết quả trong view gốc, đo Undo, hoàn thiện tài liệu.
