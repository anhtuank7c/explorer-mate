#include "App/AboutDialog.h"

#include <windows.h>

#include <commctrl.h>
#include <shellapi.h>

#include <string>

#include "App/AppIcon.h"
#include "App/ChangelogDialog.h"
#include "App/resource.h"
#include "Domain/ProductInfo.h"

namespace et::ui {

namespace {

std::wstring Text(std::wstring_view view) {
    return std::wstring(view);
}

void FillTexts(HWND dialog) {
    const std::wstring title =
        Text(domain::ProductName()) + L" " + Text(domain::ProductVersion());
    const std::wstring author = L"Author: " + Text(domain::ProductAuthor());
    // SysLink markup: the part inside <a> is shown as a link.
    const std::wstring website = L"Website: <a>" + Text(domain::ProductWebsiteLabel()) + L"</a>";
    const std::wstring repository =
        L"Source code: <a>" + Text(domain::ProductRepositoryLabel()) + L"</a>";
    SetDlgItemTextW(dialog, IDC_ABOUT_TITLE, title.c_str());
    SetDlgItemTextW(dialog, IDC_ABOUT_AUTHOR, author.c_str());
    SetDlgItemTextW(dialog, IDC_ABOUT_LINK, website.c_str());
    SetDlgItemTextW(dialog, IDC_ABOUT_REPOSITORY, repository.c_str());
    SetDlgItemTextW(dialog, IDC_ABOUT_ISSUES, L"Found a problem? <a>Report it on GitHub</a>");
}

// The page a link control opens; empty for any other control. The URLs are compiled-in
// constants, never text taken from the control.
std::wstring_view UrlOf(UINT_PTR controlId) {
    switch (controlId) {
        case IDC_ABOUT_LINK:
            return domain::ProductWebsiteUrl();
        case IDC_ABOUT_REPOSITORY:
            return domain::ProductRepositoryUrl();
        case IDC_ABOUT_ISSUES:
            return domain::ProductIssuesUrl();
        default:
            return {};
    }
}

// Opens the page in the default browser.
void OpenPage(HWND dialog, std::wstring_view url) {
    ShellExecuteW(dialog, L"open", Text(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

INT_PTR CALLBACK AboutProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            FillTexts(dialog);
            ApplyAppIcon(dialog);
            SetForegroundWindow(dialog);
            return TRUE;
        case WM_NOTIFY: {
            const auto* header = reinterpret_cast<const NMHDR*>(lParam);
            const std::wstring_view url = UrlOf(header->idFrom);
            if (!url.empty() && (header->code == NM_CLICK || header->code == NM_RETURN)) {
                OpenPage(dialog, url);
                return TRUE;
            }
            break;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_ABOUT_CHANGELOG) {
                ShowChangelog(dialog);
                return TRUE;
            }
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(dialog, LOWORD(wParam));
                return TRUE;
            }
            break;
        default:
            break;
    }
    return FALSE;
}

}  // namespace

void ShowAbout() {
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LINK_CLASS | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_ABOUT), nullptr, AboutProc, 0);
}

}  // namespace et::ui
