#include "App/SettingsDialog.h"

#include <windows.h>

#include <commctrl.h>

#include <array>
#include <string>
#include <utility>

#include "App/AppIcon.h"
#include "App/resource.h"

namespace et::ui {

namespace {

struct HotkeyField {
    int controlId;
    app::ActionKind action;
};

constexpr std::array<HotkeyField, 3> kFields{{
    {IDC_HOTKEY_GROUP, app::ActionKind::GroupIntoNewFolder},
    {IDC_HOTKEY_RENAME, app::ActionKind::BulkRename},
    {IDC_HOTKEY_DUPLICATE, app::ActionKind::DuplicateInPlace},
}};

struct DialogState {
    app::Settings settings;
};

// The hotkey control knows Ctrl, Alt and Shift but not the Win key. A stored Win shortcut is
// shown without it; saving the dialog then stores what is shown.
void ShowChord(HWND dialog, int controlId, const std::optional<domain::KeyChord>& chord) {
    WORD value = 0;
    if (chord) {
        BYTE modifiers = 0;
        if (chord->ctrl) modifiers |= HOTKEYF_CONTROL;
        if (chord->alt) modifiers |= HOTKEYF_ALT;
        if (chord->shift) modifiers |= HOTKEYF_SHIFT;
        value = MAKEWORD(static_cast<BYTE>(chord->key), modifiers);
    }
    SendDlgItemMessageW(dialog, controlId, HKM_SETHOTKEY, value, 0);
}

// nullopt when the box is empty.
std::optional<domain::KeyChord> ReadChord(HWND dialog, int controlId) {
    const WORD value = static_cast<WORD>(SendDlgItemMessageW(dialog, controlId, HKM_GETHOTKEY, 0, 0));
    const BYTE key = LOBYTE(value);
    const BYTE modifiers = HIBYTE(value);
    if (key == 0) {
        return std::nullopt;
    }
    domain::KeyChord chord;
    chord.ctrl = (modifiers & HOTKEYF_CONTROL) != 0;
    chord.alt = (modifiers & HOTKEYF_ALT) != 0;
    chord.shift = (modifiers & HOTKEYF_SHIFT) != 0;
    chord.key = key;
    return chord;
}

app::Settings ReadSettings(HWND dialog) {
    app::Settings settings;
    settings.hotkeysEnabled = IsDlgButtonChecked(dialog, IDC_HOTKEYS_ENABLED) == BST_CHECKED;
    for (const HotkeyField& field : kFields) {
        if (const auto chord = ReadChord(dialog, field.controlId)) {
            settings.hotkeys.push_back({field.action, *chord});
        }
    }
    return settings;
}

INT_PTR CALLBACK SettingsProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        SetWindowLongPtrW(dialog, DWLP_USER, lParam);
    }
    auto* state = reinterpret_cast<DialogState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (state == nullptr) {
        return FALSE;
    }
    switch (message) {
        case WM_INITDIALOG:
            CheckDlgButton(dialog, IDC_HOTKEYS_ENABLED,
                           state->settings.hotkeysEnabled ? BST_CHECKED : BST_UNCHECKED);
            for (const HotkeyField& field : kFields) {
                ShowChord(dialog, field.controlId, state->settings.ChordFor(field.action));
            }
            ApplyAppIcon(dialog);
            SetForegroundWindow(dialog);
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                app::Settings edited = ReadSettings(dialog);
                if (const auto problem = app::ValidateSettings(edited)) {
                    SetDlgItemTextW(dialog, IDC_ERROR, problem->message.c_str());
                    return TRUE;
                }
                state->settings = std::move(edited);
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

}  // namespace

std::optional<app::Settings> EditSettings(const app::Settings& current) {
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_HOTKEY_CLASS | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    DialogState state{current};
    if (DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_SETTINGS), nullptr,
                        SettingsProc, reinterpret_cast<LPARAM>(&state)) != IDOK) {
        return std::nullopt;
    }
    return state.settings;
}

}  // namespace et::ui
