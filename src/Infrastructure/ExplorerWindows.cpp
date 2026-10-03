#include "Infrastructure/ExplorerWindows.h"

#include <oleauto.h>

#include <exdisp.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl_core.h>
#include <wrl/client.h>

#include "Infrastructure/ShellSelection.h"

namespace et::infra {

namespace {

using Microsoft::WRL::ComPtr;

constexpr const wchar_t* kTabWindowClass = L"ShellTabWindowClass";

std::wstring ClassNameOf(HWND window) {
    wchar_t name[128]{};
    const int length = window != nullptr ? GetClassNameW(window, name, 128) : 0;
    return std::wstring(name, static_cast<size_t>(length));
}

std::wstring NameOf(PCIDLIST_ABSOLUTE folder, SIGDN form) {
    PWSTR raw = nullptr;
    if (FAILED(SHGetNameFromIDList(folder, form, &raw))) {
        return {};
    }
    std::wstring name(raw);
    CoTaskMemFree(raw);
    return name;
}

void ReadFolder(IFolderView2* view, ExplorerTab& tab) {
    ComPtr<IPersistFolder2> folder;
    PIDLIST_ABSOLUTE location = nullptr;
    if (FAILED(view->GetFolder(IID_PPV_ARGS(&folder))) || FAILED(folder->GetCurFolder(&location))) {
        return;
    }
    tab.folder = NameOf(location, SIGDN_FILESYSPATH);
    tab.location = NameOf(location, SIGDN_DESKTOPABSOLUTEPARSING);
    CoTaskMemFree(location);
}

void ReadSelection(IFolderView2* view, ExplorerTab& tab) {
    // GetSelection fails outright when nothing is selected, so ask for the count first to
    // tell "empty selection" apart from "could not read".
    int selectedCount = 0;
    if (FAILED(view->ItemCount(SVGIO_SELECTION, &selectedCount))) {
        return;
    }
    if (selectedCount == 0) {
        tab.selectionReadable = true;
        return;
    }
    ComPtr<IShellItemArray> items;
    if (FAILED(view->GetSelection(FALSE, &items)) || !items) {
        return;
    }
    auto paths = ReadFileSystemPaths(items.Get());
    tab.selectionReadable = paths.ok();
    if (paths.ok()) {
        tab.selection = std::move(paths).value();
    }
}

// Fills `tab` from one IShellWindows entry. False when the entry is not an Explorer tab
// (IShellWindows also lists legacy Internet Explorer windows).
bool DescribeTab(IDispatch* entry, ExplorerTab& tab) {
    ComPtr<IWebBrowserApp> application;
    SHANDLE_PTR frame = 0;
    if (FAILED(entry->QueryInterface(IID_PPV_ARGS(&application))) ||
        FAILED(application->get_HWND(&frame))) {
        return false;
    }
    ComPtr<IShellBrowser> browser;
    ComPtr<IShellView> shellView;
    if (FAILED(IUnknown_QueryService(entry, SID_STopLevelBrowser, IID_PPV_ARGS(&browser))) ||
        FAILED(browser->QueryActiveShellView(&shellView))) {
        return false;
    }

    tab.frame = reinterpret_cast<HWND>(frame);
    browser->GetWindow(&tab.tabWindow);
    shellView->GetWindow(&tab.viewWindow);
    // The frame keeps the shown tab's host window first among its tab-class children.
    tab.isActiveTab = tab.tabWindow != nullptr &&
                      FindWindowExW(tab.frame, nullptr, kTabWindowClass, nullptr) == tab.tabWindow;

    ComPtr<IFolderView2> folderView;
    if (SUCCEEDED(shellView.As(&folderView))) {
        if (FAILED(folderView->ItemCount(SVGIO_ALLVIEW, &tab.itemsInView))) {
            tab.itemsInView = -1;
        }
        ReadFolder(folderView.Get(), tab);
        ReadSelection(folderView.Get(), tab);
    }
    return true;
}

}  // namespace

std::vector<ExplorerTab> EnumerateExplorerTabs() {
    std::vector<ExplorerTab> tabs;
    ComPtr<IShellWindows> windows;
    long count = 0;
    if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&windows))) ||
        FAILED(windows->get_Count(&count))) {
        return tabs;
    }
    for (long index = 0; index < count; ++index) {
        VARIANT position;
        VariantInit(&position);
        position.vt = VT_I4;
        position.lVal = index;
        ComPtr<IDispatch> entry;
        ExplorerTab tab;
        if (windows->Item(position, &entry) == S_OK && entry && DescribeTab(entry.Get(), tab)) {
            tabs.push_back(std::move(tab));
        }
    }
    return tabs;
}

ForegroundFocus ReadForegroundFocus() {
    ForegroundFocus state;
    state.foreground = GetForegroundWindow();
    state.foregroundClass = ClassNameOf(state.foreground);
    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    if (state.foreground != nullptr &&
        GetGUIThreadInfo(GetWindowThreadProcessId(state.foreground, nullptr), &info)) {
        state.focus = info.hwndFocus;
        state.focusClass = ClassNameOf(state.focus);
        for (HWND window = state.focus; window != nullptr; window = GetAncestor(window, GA_PARENT)) {
            if (window == GetDesktopWindow()) {
                break;
            }
            state.focusAncestry += (state.focusAncestry.empty() ? L"" : L" < ") + ClassNameOf(window);
        }
    }
    return state;
}

}  // namespace et::infra
