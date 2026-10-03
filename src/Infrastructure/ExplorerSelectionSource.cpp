#include "Infrastructure/ExplorerSelectionSource.h"

#include <windows.h>

#include <string_view>

#include "Infrastructure/ExplorerWindows.h"

namespace et::infra {

namespace {

constexpr std::wstring_view kExplorerFrameClass = L"CabinetWClass";
constexpr std::wstring_view kShellViewClass = L"SHELLDLL_DefView";
// The file list itself. While an item is being renamed the focus moves to an "Edit" child
// of this window, which is how typing in a rename box is told apart from the list.
constexpr std::wstring_view kFileListClass = L"DirectUIHWND";

bool HasClass(HWND window, std::wstring_view expected) {
    wchar_t name[64]{};
    const int length = window != nullptr ? GetClassNameW(window, name, 64) : 0;
    return std::wstring_view(name, static_cast<size_t>(length)) == expected;
}

// The shell view window owning the focused file list, or null when the focus is elsewhere.
// Only cheap user32 calls: this runs inside the low-level keyboard hook.
HWND FocusedShellView() {
    const HWND foreground = GetForegroundWindow();
    if (!HasClass(foreground, kExplorerFrameClass)) {
        return nullptr;
    }
    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    if (!GetGUIThreadInfo(GetWindowThreadProcessId(foreground, nullptr), &info) ||
        !HasClass(info.hwndFocus, kFileListClass)) {
        return nullptr;
    }
    const HWND view = GetAncestor(info.hwndFocus, GA_PARENT);
    return HasClass(view, kShellViewClass) ? view : nullptr;
}

domain::Error Refused(const wchar_t* reason) {
    return domain::Error(domain::ErrorCode::UnsupportedLocation, reason);
}

}  // namespace

bool ExplorerSelectionSource::FocusIsInFileList() const {
    return FocusedShellView() != nullptr;
}

domain::Result<std::vector<std::wstring>> ExplorerSelectionSource::CaptureFocusedSelection() const {
    const HWND focusedView = FocusedShellView();
    if (focusedView == nullptr) {
        return Refused(L"The keyboard focus is not in a File Explorer file list.");
    }
    const HWND foreground = GetForegroundWindow();

    const ExplorerTab* focusedTab = nullptr;
    size_t shownTabs = 0;
    const std::vector<ExplorerTab> tabs = EnumerateExplorerTabs();
    for (const ExplorerTab& tab : tabs) {
        if (tab.frame == foreground && tab.isActiveTab) {
            ++shownTabs;
            focusedTab = &tab;
        }
    }
    if (shownTabs != 1) {
        return Refused(L"Could not tell which File Explorer tab is active.");
    }
    if (focusedTab->viewWindow != focusedView) {
        return Refused(L"The focused file list does not belong to the active tab.");
    }
    // The user may have switched windows while the tabs were being read.
    if (FocusedShellView() != focusedView) {
        return Refused(L"The focus moved while reading the selection.");
    }
    if (focusedTab->folder.empty() || !focusedTab->selectionReadable) {
        return Refused(L"This location is not a regular folder.");
    }
    if (focusedTab->selection.empty()) {
        return domain::Error(domain::ErrorCode::EmptySelection, L"Nothing is selected.");
    }
    return focusedTab->selection;
}

}  // namespace et::infra
