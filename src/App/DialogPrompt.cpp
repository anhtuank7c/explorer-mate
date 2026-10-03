#include "App/DialogPrompt.h"

#include <windows.h>

#include <commctrl.h>

#include <algorithm>
#include <cwchar>
#include <string>

#include "App/resource.h"

namespace et::ui {

namespace {

// Filling a list view on every keystroke must stay instant even for huge selections.
constexpr size_t kMaxPreviewRows = 300;

std::wstring TextOf(HWND dialog, int controlId) {
    const HWND control = GetDlgItem(dialog, controlId);
    std::wstring text(static_cast<size_t>(GetWindowTextLengthW(control)) + 1, L'\0');
    const int length = GetWindowTextW(control, text.data(), static_cast<int>(text.size()));
    text.resize(static_cast<size_t>(length));
    return text;
}

// The worker is started by a background COM host, so its first window does not get the
// foreground by default and would open behind File Explorer.
// Passing through "topmost" raises it above Explorer even when Windows refuses to hand over
// keyboard focus; it does not stay topmost.
void BringToFront(HWND dialog) {
    SetWindowPos(dialog, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetForegroundWindow(dialog);
    if (GetForegroundWindow() != dialog) {
        // Refused, which is the normal case when started by a keyboard shortcut. Joining the
        // input queue of the current foreground thread for a moment makes Windows treat the
        // request as coming from the window the user is working in.
        const DWORD self = GetCurrentThreadId();
        const DWORD foregroundThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
        if (foregroundThread != 0 && foregroundThread != self &&
            AttachThreadInput(self, foregroundThread, TRUE)) {
            SetForegroundWindow(dialog);
            AttachThreadInput(self, foregroundThread, FALSE);
        }
    }
    SetWindowPos(dialog, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void ShowValidation(HWND dialog, const std::wstring& problem) {
    SetDlgItemTextW(dialog, IDC_ERROR, problem.c_str());
    EnableWindow(GetDlgItem(dialog, IDOK), problem.empty());
}

template <typename State>
State* StateOf(HWND dialog, UINT message, LPARAM parameter) {
    if (message == WM_INITDIALOG) {
        SetWindowLongPtrW(dialog, DWLP_USER, parameter);
    }
    return reinterpret_cast<State*>(GetWindowLongPtrW(dialog, DWLP_USER));
}

// --- Folder name ------------------------------------------------------------------------

struct FolderNameState {
    const std::wstring& suggestion;
    const app::NameValidator& validate;
    std::wstring result;
};

void ValidateFolderName(HWND dialog, const FolderNameState& state) {
    ShowValidation(dialog, state.validate(TextOf(dialog, IDC_NAME)).value_or(L""));
}

INT_PTR CALLBACK FolderNameProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    FolderNameState* state = StateOf<FolderNameState>(dialog, message, lParam);
    if (state == nullptr) {
        return FALSE;
    }
    switch (message) {
        case WM_INITDIALOG:
            SetDlgItemTextW(dialog, IDC_NAME, state->suggestion.c_str());
            SendDlgItemMessageW(dialog, IDC_NAME, EM_SETSEL, 0, -1);
            ValidateFolderName(dialog, *state);
            BringToFront(dialog);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_NAME && HIWORD(wParam) == EN_CHANGE) {
                ValidateFolderName(dialog, *state);
                return TRUE;
            }
            if (LOWORD(wParam) == IDOK && IsWindowEnabled(GetDlgItem(dialog, IDOK))) {
                state->result = TextOf(dialog, IDC_NAME);
                EndDialog(dialog, IDOK);
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(dialog, IDCANCEL);
                return TRUE;
            }
            break;
        default:
            break;
    }
    return FALSE;
}

// --- Bulk rename ------------------------------------------------------------------------

struct RenameState {
    const domain::RenamePattern& suggestion;
    const app::RenamePreviewer& preview;
    domain::RenamePattern result;
};

void AddColumn(HWND list, int index, const wchar_t* title, int width) {
    LVCOLUMNW column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    column.pszText = const_cast<wchar_t*>(title);
    column.cx = width;
    ListView_InsertColumn(list, index, &column);
}

void AddRow(HWND list, int row, const std::wstring& left, const std::wstring& right) {
    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.pszText = const_cast<wchar_t*>(left.c_str());
    ListView_InsertItem(list, &item);
    ListView_SetItemText(list, row, 1, const_cast<wchar_t*>(right.c_str()));
}

void ShowPreview(HWND dialog, const std::vector<domain::RenamePreview>& previews) {
    const HWND list = GetDlgItem(dialog, IDC_PREVIEW);
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(list);
    const size_t shown = std::min(previews.size(), kMaxPreviewRows);
    for (size_t row = 0; row < shown; ++row) {
        AddRow(list, static_cast<int>(row), previews[row].original, previews[row].renamed);
    }
    if (shown < previews.size()) {
        AddRow(list, static_cast<int>(shown),
               L"... and " + std::to_wstring(previews.size() - shown) + L" more", L"");
    }
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, TRUE);
}

// Reads a number field. Returns false when it is empty; the edit controls only accept digits
// and are length-limited, so nothing else can go wrong.
bool ReadNumber(HWND dialog, int controlId, unsigned& value) {
    const std::wstring text = TextOf(dialog, controlId);
    if (text.empty()) {
        return false;
    }
    value = static_cast<unsigned>(std::wcstoul(text.c_str(), nullptr, 10));
    return true;
}

// Reads the fields into `pattern`. Returns the problem to show when a field is unusable.
std::wstring ReadPattern(HWND dialog, domain::RenamePattern& pattern) {
    pattern.nameMask = TextOf(dialog, IDC_NAME_MASK);
    pattern.extensionMask = TextOf(dialog, IDC_EXTENSION_MASK);
    pattern.searchFor = TextOf(dialog, IDC_SEARCH);
    pattern.replaceWith = TextOf(dialog, IDC_REPLACE);
    if (!ReadNumber(dialog, IDC_START, pattern.counterStart) ||
        !ReadNumber(dialog, IDC_STEP, pattern.counterStep) ||
        !ReadNumber(dialog, IDC_DIGITS, pattern.counterDigits)) {
        return L"Enter the counter's start, step and number of digits.";
    }
    return {};
}

// Inserts a placeholder at the caret of the file-name mask and returns the focus there, so
// the user can keep typing. With a selection start/end, that part of the inserted text is
// left selected for the user to overwrite.
void InsertIntoNameMask(HWND dialog, const wchar_t* text, int selectFrom = 0, int selectTo = 0) {
    const HWND edit = GetDlgItem(dialog, IDC_NAME_MASK);
    DWORD caret = 0;
    SendMessageW(edit, EM_GETSEL, reinterpret_cast<WPARAM>(&caret), 0);
    SendMessageW(edit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(text));
    if (selectTo > selectFrom) {
        SendMessageW(edit, EM_SETSEL, caret + selectFrom, caret + selectTo);
    }
    SetFocus(edit);
}

bool HandleInsertButton(HWND dialog, int controlId) {
    switch (controlId) {
        case IDC_INSERT_NAME:
            InsertIntoNameMask(dialog, L"[N]");
            return true;
        case IDC_INSERT_RANGE:
            InsertIntoNameMask(dialog, L"[N1-5]", 2, 5);
            return true;
        case IDC_INSERT_COUNTER:
            InsertIntoNameMask(dialog, L"[C]");
            return true;
        case IDC_INSERT_PARENT:
            InsertIntoNameMask(dialog, L"[P]");
            return true;
        default:
            return false;
    }
}

void RefreshRename(HWND dialog, RenameState& state) {
    std::wstring problem = ReadPattern(dialog, state.result);
    if (problem.empty()) {
        const auto previews = state.preview(state.result);
        if (previews.ok()) {
            ShowPreview(dialog, previews.value());
        } else {
            problem = previews.error().message;
        }
    }
    ShowValidation(dialog, problem);
}

void InitRenameDialog(HWND dialog, RenameState& state) {
    const HWND list = GetDlgItem(dialog, IDC_PREVIEW);
    ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    RECT area{};
    GetClientRect(list, &area);
    const int columnWidth = (area.right - GetSystemMetrics(SM_CXVSCROLL)) / 2;
    AddColumn(list, 0, L"Current name", columnWidth);
    AddColumn(list, 1, L"New name", columnWidth);

    SendDlgItemMessageW(dialog, IDC_START, EM_SETLIMITTEXT, 6, 0);
    SendDlgItemMessageW(dialog, IDC_STEP, EM_SETLIMITTEXT, 6, 0);
    SendDlgItemMessageW(dialog, IDC_DIGITS, EM_SETLIMITTEXT, 1, 0);
    const domain::RenamePattern& suggestion = state.suggestion;
    SetDlgItemTextW(dialog, IDC_NAME_MASK, suggestion.nameMask.c_str());
    SetDlgItemTextW(dialog, IDC_EXTENSION_MASK, suggestion.extensionMask.c_str());
    SetDlgItemTextW(dialog, IDC_START, std::to_wstring(suggestion.counterStart).c_str());
    SetDlgItemTextW(dialog, IDC_STEP, std::to_wstring(suggestion.counterStep).c_str());
    SetDlgItemTextW(dialog, IDC_DIGITS, std::to_wstring(suggestion.counterDigits).c_str());
    SetDlgItemTextW(dialog, IDC_SEARCH, suggestion.searchFor.c_str());
    SetDlgItemTextW(dialog, IDC_REPLACE, suggestion.replaceWith.c_str());
    RefreshRename(dialog, state);
    BringToFront(dialog);
}

INT_PTR CALLBACK RenameProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    RenameState* state = StateOf<RenameState>(dialog, message, lParam);
    if (state == nullptr) {
        return FALSE;
    }
    switch (message) {
        case WM_INITDIALOG:
            InitRenameDialog(dialog, *state);
            return TRUE;
        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE) {
                RefreshRename(dialog, *state);
                return TRUE;
            }
            if (HIWORD(wParam) == BN_CLICKED && HandleInsertButton(dialog, LOWORD(wParam))) {
                return TRUE;
            }
            if (LOWORD(wParam) == IDOK && IsWindowEnabled(GetDlgItem(dialog, IDOK))) {
                EndDialog(dialog, IDOK);
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(dialog, IDCANCEL);
                return TRUE;
            }
            break;
        default:
            break;
    }
    return FALSE;
}

INT_PTR ShowDialog(int templateId, DLGPROC procedure, void* state) {
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    return DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(templateId), nullptr,
                           procedure, reinterpret_cast<LPARAM>(state));
}

}  // namespace

std::optional<std::wstring> DialogPrompt::AskFolderName(const std::wstring& suggestion,
                                                        const app::NameValidator& validate) {
    FolderNameState state{suggestion, validate, {}};
    if (ShowDialog(IDD_FOLDER_NAME, FolderNameProc, &state) != IDOK) {
        return std::nullopt;
    }
    return state.result;
}

std::optional<domain::RenamePattern> DialogPrompt::AskRenamePattern(
    const domain::RenamePattern& suggestion, const app::RenamePreviewer& preview) {
    RenameState state{suggestion, preview, suggestion};
    if (ShowDialog(IDD_BULK_RENAME, RenameProc, &state) != IDOK) {
        return std::nullopt;
    }
    return state.result;
}

}  // namespace et::ui
