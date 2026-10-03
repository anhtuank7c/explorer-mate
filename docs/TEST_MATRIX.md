# Ma trận kiểm thử

Môi trường đã chạy: Windows 11 Pro build 26300 x64, Visual Studio Community 18.10.3, MSVC 14.51.36231, SDK 10.0.26100.0. Cập nhật: 03/10/2026.

Trạng thái: **PASS** (đã chạy, đạt), **FAIL**, **NOT RUN** (chưa chạy — không được coi là đạt).

## Tự động

Chạy bằng `scripts\test.ps1` trên cả Debug và Release: 113/113 PASS, cả trên máy dev lẫn trên CI của GitHub (runner `windows-2025-vs2026`). Các số theo nhóm bên dưới là của đợt đầu (89 test); sau đó thêm 18 test cho `KeyChord`, `Settings`, `HotkeyMatcher` và 6 test từ đợt review bảo mật (đường dẫn, phiên bản, veto phím tắt, mã hóa request, logger).

Quét tự động trên CI cho mỗi lần push: MSVC `/analyze` (0 phát hiện), CodeQL bộ `security-extended` (0 cảnh báo), BinSkim (0 lỗi; 2 cảnh báo được chấp nhận trên mỗi file: BA2024 Spectre, BA2027 SourceLink).

| Nhóm | Phạm vi | Số test |
|---|---|---|
| Unit — Domain | Tách stem/extension, luật tên Windows, đường dẫn, collation, selection, rename mask (placeholder, dải ký tự, mask sai), kế hoạch đổi tên (bộ đếm, tìm–thay thế, thứ tự, trùng, chuỗi phụ thuộc, vòng) | 41 |
| Unit — Application | Ba use case với filesystem giả: thành công, hủy, lỗi từng phần, race tạo folder, selection không hợp lệ; định dạng request | 24 |
| Integration — Shell | `IFileOperation` thật trên `%TEMP%\ExplorerMate.Tests`: duplicate file/cây thư mục, move, đổi tên chuỗi/hoán đổi, dừng ở lỗi đầu, không ghi đè; probe; collation Windows; use case gom nhóm đầu-cuối | 15 |
| Integration — khác | Request file (ngoài thư mục, sai đuôi, UTF-8 hỏng, dọn file cũ), logger, COM apartment, kích hoạt ba lệnh qua `DllGetClassObject` | 9 |

## Bán tự động (script điều khiển bản đã cài)

| Tình huống | Cách chạy | Kết quả |
|---|---|---|
| Đăng ký / gỡ / đăng ký lại package | `install-dev.ps1`, `uninstall-dev.ps1` | PASS |
| Duplicate qua COM như Explorer gọi (file + folder) | `probe-command.ps1` | PASS — `a - Copy.txt`, `Assets - Copy\` |
| Bulk rename ẩn khi chọn folder, hiện khi chọn file | `probe-command.ps1` | PASS — `ECS_HIDDEN` / `ECS_ENABLED` |
| Request file được dọn sau khi dùng | đếm file trong `requests` | PASS — 0 |
| Dialog gom nhóm: gợi ý, tên trùng, ký tự cấm, OK, Cancel | `probe-dialog.ps1` | PASS |
| Dialog đổi tên (bản mask): mặc định `[N]_[C]`, mask sai, ô số trống, mask thiếu bộ đếm, nút chèn, OK với mặc định, OK với `[P] [C]` + start/step/digits + tìm–thay thế, Cancel | `probe-dialog.ps1` | PASS |
| CLI: duplicate, group, hủy, từ chối folder | `ExplorerMate.exe --action ... --silent` | PASS |
| CLI: rename với cờ mask (`--mask "[C]-[N1-1]" --ext-mask md --start 5 --step 5 --digits 3` → `005-a.md`, `010-b.md`; mask sai → exit 1) | `ExplorerMate.exe --action rename --silent` | PASS |

### Giai đoạn 2 (phím tắt)

| Tình huống | Cách chạy | Kết quả |
|---|---|---|
| Ba tab trong một frame, hai tab cùng thư mục khác selection: nhận đúng tab active và selection | `test-tab-detection.ps1` | PASS |
| Nhiều cửa sổ Explorer cùng lúc (cửa sổ thử + cửa sổ của người dùng) | `test-tab-detection.ps1` | PASS |
| Focus ở file list → đích phím tắt = selection của tab active | `test-tab-detection.ps1` | PASS |
| Focus ở ô đổi tên (F2) → phím tắt bị từ chối | `test-tab-detection.ps1` | PASS |
| Focus ở thanh địa chỉ / ô tìm kiếm / khung điều hướng → từ chối | — | NOT RUN |
| Đổi tab đúng lúc bấm phím | — | NOT RUN |
| Agent: khởi động, single instance, `--stop-agent` | lệnh trực tiếp | PASS |
| Sau đợt review bảo mật: agent chỉ hành động khi có phím thật (thông điệp từ tiến trình khác không mang action); đối chiếu modifier với trạng thái bàn phím thật | — | NOT RUN bằng phím thật sau khi sửa (logic veto có unit test). Script `test-agent-hotkey.ps1` đã bị xóa vì nó dựa đúng vào lỗ hổng vừa được vá |
| Bấm phím thật Ctrl+Alt+N / R / D trong file list | người dùng | PASS (03/10/2026: "it all works"; các dòng "người dùng" còn lại trong bảng này được yêu cầu thử cùng lúc nhưng không được xác nhận riêng từng dòng) |
| Phím tắt trong ứng dụng khác, ô tìm kiếm, thanh địa chỉ không bị chặn | người dùng | NOT RUN |
| Giữ phím (auto-repeat), nhả phím, AltGr | unit test `HotkeyMatcherTests` | PASS (logic); phím thật NOT RUN |
| Menu khay, dialog Settings, đổi phím, Start with Windows | người dùng | NOT RUN |
| Dialog của worker mở phía trước khi gọi bằng phím tắt | người dùng | NOT RUN |

## Thủ công trong Explorer

| Nhóm | Tình huống | Kết quả |
|---|---|---|
| Menu | Lệnh thử nghiệm hiện ở menu chính; bấm với 1 file ghi đúng selection (bản M1) | PASS (người dùng, 03/10/2026) |
| Group, Duplicate | Dùng từ menu chuột phải | PASS (người dùng, 03/10/2026: "hoạt động đúng kỳ vọng"; không ghi chi tiết từng tình huống) |
| Rename (bản mask) | Dùng từ menu chuột phải, nhìn bố cục dialog | PASS (người dùng, 03/10/2026: "bulk rename work great"; không ghi chi tiết từng tình huống) |
| Menu | Ba lệnh thật hiện ở menu chính với 1 file, nhiều file, folder, mixed | NOT RUN (chi tiết từng loại selection) |
| Menu | Bulk rename không hiện khi selection có folder | NOT RUN |
| Dialog | Mở phía trước Explorer, nhận bàn phím ngay, Enter/Escape, tab order | NOT RUN |
| Dialog | Hiển thị đúng ở DPI 125%/150% | NOT RUN |
| Group | Chọn file + folder, đặt tên tiếng Việt, kiểm tra nội dung folder mới | NOT RUN |
| Rename | Giữ tên gốc; tên chung; chạy lại trên lô đã đổi | NOT RUN |
| Duplicate | File; folder; bấm hai lần liên tiếp | NOT RUN |
| Progress | File vài GB, cây nhiều file, bấm Cancel giữa chừng | NOT RUN |
| Undo | Ctrl+Z sau Duplicate, sau Group, sau Rename — ghi số bước thực tế | NOT RUN |
| Failure | File đang mở bởi ứng dụng khác; thư mục chỉ đọc | NOT RUN |
| Paths | Đường dẫn dài >260; OneDrive online-only; UNC; junction | NOT RUN (junction/cloud/UNC bị từ chối theo thiết kế, đã có unit test) |
| Package | Chạy bản Release khi không mở Visual Studio; gỡ rồi kiểm tra menu biến mất | NOT RUN |

## Hành vi đã đo

- DLL menu chạy trong `DllHost.exe`, không phải `explorer.exe`.
- Thiếu `FileSystemWriteVirtualization=disabled` thì `%LOCALAPPDATA%` của DLL bị chuyển hướng vào thư mục package.
- Tên bản sao do Shell đặt: `<tên> - Copy<đuôi>` (Windows tiếng Anh).
- `FOF_SILENT | FOF_NOERRORUI` không chặn hộp thoại "Replace or Skip"; gateway chặn trước.
- Undo: chưa đo.
