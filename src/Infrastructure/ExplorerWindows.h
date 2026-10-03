#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace et::infra {

// One File Explorer tab as seen through IShellWindows. Several tabs share one frame window,
// so the frame handle alone never identifies a tab.
struct ExplorerTab {
    HWND frame = nullptr;       // Top-level Explorer window.
    HWND tabWindow = nullptr;   // Per-tab host window reported by the tab's IShellBrowser.
    HWND viewWindow = nullptr;  // The tab's file list (shell view) window.
    bool isActiveTab = false;   // The tab currently shown in its frame.
    std::wstring folder;        // Filesystem path; empty for virtual folders (Home, Search...).
    std::wstring location;      // Display/parsing name, also set for virtual folders.
    bool selectionReadable = false;        // False when an item has no filesystem path.
    std::vector<std::wstring> selection;   // Filesystem paths of the selected items.
};

// Every open Explorer tab. The calling thread must be in a COM apartment.
std::vector<ExplorerTab> EnumerateExplorerTabs();

// Keyboard focus of the foreground window, for deciding whether a hotkey belongs to the
// file list or to a text field (rename box, address bar, search box).
struct ForegroundFocus {
    HWND foreground = nullptr;
    HWND focus = nullptr;
    std::wstring foregroundClass;
    std::wstring focusClass;
    // Window classes from the focused window up to the top-level window, e.g.
    // "DirectUIHWND < SHELLDLL_DefView < ... < CabinetWClass".
    std::wstring focusAncestry;
};

ForegroundFocus ReadForegroundFocus();

}  // namespace et::infra
