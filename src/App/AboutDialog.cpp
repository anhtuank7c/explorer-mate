#include "App/AboutDialog.h"

#include <windows.h>

#include <commctrl.h>
#include <shellapi.h>

#include <string>

#include "App/AppIcon.h"
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
    // SysLink markup; the control shows the label and reports the href when clicked.
    const std::wstring link = L"<a href=\"" + Text(domain::ProductWebsiteUrl()) + L"\">" +
                              Text(domain::ProductWebsiteLabel()) + L"</a>";
    SetDlgItemTextW(dialog, IDC_ABOUT_TITLE, title.c_str());
    SetDlgItemTextW(dialog, IDC_ABOUT_AUTHOR, author.c_str());
    SetDlgItemTextW(dialog, IDC_ABOUT_LINK, link.c_str());
}

// Opens the product website in the default browser. The URL is the compiled-in constant,
// never text taken from the control.
void OpenWebsite(HWND dialog) {
    ShellExecuteW(dialog, L"open", Text(domain::ProductWebsiteUrl()).c_str(), nullptr, nullptr,
                  SW_SHOWNORMAL);
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
            if (header->idFrom == IDC_ABOUT_LINK &&
                (header->code == NM_CLICK || header->code == NM_RETURN)) {
                OpenWebsite(dialog);
                return TRUE;
            }
            break;
        }
        case WM_COMMAND:
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
