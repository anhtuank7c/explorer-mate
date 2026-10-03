#include "App/ExplorerDiagnostics.h"

#include <windows.h>

#include <cwchar>
#include <string>

#include "App/CommandLine.h"
#include "Domain/PathText.h"
#include "Infrastructure/ExplorerSelectionSource.h"
#include "Infrastructure/ExplorerWindows.h"

namespace et::ui {

namespace {

std::wstring Hex(HWND window) {
    wchar_t text[24]{};
    swprintf_s(text, L"0x%08llX", static_cast<unsigned long long>(reinterpret_cast<UINT_PTR>(window)));
    return text;
}

std::wstring ClassOf(HWND window) {
    wchar_t name[128]{};
    const int length = window != nullptr ? GetClassNameW(window, name, 128) : 0;
    return std::wstring(name, static_cast<size_t>(length));
}

const wchar_t* YesNo(bool value) {
    return value ? L"yes" : L"no";
}

void PrintTab(const infra::ExplorerTab& tab, const infra::ForegroundFocus& focus) {
    const bool focusInView = focus.focus != nullptr && tab.viewWindow != nullptr &&
                             (focus.focus == tab.viewWindow || IsChild(tab.viewWindow, focus.focus));
    WriteLineToStdout(L"tab frame=" + Hex(tab.frame) + L" tabWindow=" + Hex(tab.tabWindow) + L" (" +
                      ClassOf(tab.tabWindow) + L") view=" + Hex(tab.viewWindow) + L" (" +
                      ClassOf(tab.viewWindow) + L")");
    WriteLineToStdout(std::wstring(L"  activeInFrame=") + YesNo(tab.isActiveTab) +
                      L" frameIsForeground=" + YesNo(tab.frame == focus.foreground) +
                      L" viewVisible=" + YesNo(IsWindowVisible(tab.viewWindow) != FALSE) +
                      L" focusInsideView=" + YesNo(focusInView));
    WriteLineToStdout(L"  folder=" + (tab.folder.empty() ? L"(virtual) " + tab.location : tab.folder));
    WriteLineToStdout(std::wstring(L"  selectionReadable=") + YesNo(tab.selectionReadable) +
                      L" selected=" + std::to_wstring(tab.selection.size()) +
                      L" itemsInView=" + std::to_wstring(tab.itemsInView));
    for (const std::wstring& path : tab.selection) {
        WriteLineToStdout(L"    " + std::wstring(domain::NameOf(path)));
    }
}

// One line describing what a hotkey pressed right now would act on.
std::wstring DescribeMoment() {
    const infra::ForegroundFocus focus = infra::ReadForegroundFocus();
    std::wstring line = L"fg=" + focus.foregroundClass + L" | focus=" + focus.focusAncestry;

    size_t tabsInFrame = 0;
    size_t activeTabs = 0;
    const infra::ExplorerTab* active = nullptr;
    const auto tabs = infra::EnumerateExplorerTabs();
    for (const infra::ExplorerTab& tab : tabs) {
        if (tab.frame != focus.foreground) {
            continue;
        }
        ++tabsInFrame;
        if (tab.isActiveTab) {
            ++activeTabs;
            active = &tab;
        }
    }
    line += L" | tabsInFrame=" + std::to_wstring(tabsInFrame) + L" active=" + std::to_wstring(activeTabs);
    if (activeTabs == 1) {
        const bool focusInView =
            focus.focus != nullptr && (focus.focus == active->viewWindow ||
                                       IsChild(active->viewWindow, focus.focus));
        line += std::wstring(L" focusInView=") + YesNo(focusInView) + L" | folder=" +
                (active->folder.empty() ? L"(virtual) " + active->location : active->folder) +
                L" | selected=[";
        for (size_t index = 0; index < active->selection.size(); ++index) {
            line += (index == 0 ? L"" : L", ") + std::wstring(domain::NameOf(active->selection[index]));
        }
        line += L"]";
    }
    return line;
}

}  // namespace

void WatchExplorer(unsigned seconds) {
    // Output goes to stdout only. It contains folder paths and selected file names, which
    // must not be left behind in a log file; the person running it decides where it goes.
    std::wstring previous;
    const ULONGLONG end = GetTickCount64() + static_cast<ULONGLONG>(seconds) * 1000;
    while (GetTickCount64() < end) {
        std::wstring current = DescribeMoment();
        if (current != previous) {
            WriteLineToStdout(current);
            previous = std::move(current);
        }
        Sleep(200);
    }
}

void PrintExplorerDiagnostics(unsigned delaySeconds) {
    Sleep(delaySeconds * 1000);
    const infra::ForegroundFocus focus = infra::ReadForegroundFocus();
    WriteLineToStdout(L"foreground=" + Hex(focus.foreground) + L" (" + focus.foregroundClass +
                      L") focus=" + Hex(focus.focus) + L" (" + focus.focusAncestry + L")");
    // Exactly what a keyboard shortcut pressed at this moment would be allowed to act on.
    const infra::ExplorerSelectionSource source;
    const auto target = source.CaptureFocusedSelection();
    std::wstring shortcut = std::wstring(L"shortcut focusInFileList=") +
                            YesNo(source.FocusIsInFileList()) + L" target=";
    if (target.ok()) {
        for (const std::wstring& path : target.value()) {
            shortcut += L"[" + std::wstring(domain::NameOf(path)) + L"]";
        }
    } else {
        shortcut += L"refused: " + target.error().message;
    }
    WriteLineToStdout(shortcut);

    const auto tabs = infra::EnumerateExplorerTabs();
    WriteLineToStdout(L"tabs=" + std::to_wstring(tabs.size()));
    for (const infra::ExplorerTab& tab : tabs) {
        PrintTab(tab, focus);
    }
}

}  // namespace et::ui
