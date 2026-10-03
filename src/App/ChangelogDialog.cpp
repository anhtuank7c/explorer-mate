#include "App/ChangelogDialog.h"

#include <string>
#include <string_view>

#include "App/AppIcon.h"
#include "App/resource.h"
#include "Infrastructure/Utf8.h"

namespace et::ui {

namespace {

// The bytes of CHANGELOG.md, or empty when the resource is missing.
std::string_view EmbeddedChangelog() {
    const HMODULE module = GetModuleHandleW(nullptr);
    const HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(IDR_CHANGELOG), RT_RCDATA);
    const HGLOBAL loaded = resource != nullptr ? LoadResource(module, resource) : nullptr;
    const void* bytes = loaded != nullptr ? LockResource(loaded) : nullptr;
    if (bytes == nullptr) {
        return {};
    }
    return {static_cast<const char*>(bytes), SizeofResource(module, resource)};
}

std::wstring_view TakeLine(std::wstring_view& text) {
    const size_t end = text.find(L'\n');
    std::wstring_view line = text.substr(0, end);
    text.remove_prefix(end == std::wstring_view::npos ? text.size() : end + 1);
    if (!line.empty() && line.back() == L'\r') {
        line.remove_suffix(1);
    }
    return line;
}

// Turns the Markdown into plain text for an edit control: starts at the first version
// heading, drops heading marks, the brackets around versions and code ticks, and shows list
// items with a bullet.
std::wstring ToPlainText(std::wstring_view markdown) {
    std::wstring text;
    bool started = false;
    while (!markdown.empty()) {
        std::wstring_view line = TakeLine(markdown);
        const bool versionHeading = line.substr(0, 3) == L"## ";
        started = started || versionHeading;
        if (!started) {
            continue;
        }
        if (line.substr(0, 2) == L"- ") {
            text += L"   \x2022  ";
            line.remove_prefix(2);
        } else {
            const size_t content = line.find_first_not_of(L"# ");
            line.remove_prefix(content == std::wstring_view::npos ? line.size() : content);
        }
        for (const wchar_t character : line) {
            const bool versionBracket = versionHeading && (character == L'[' || character == L']');
            if (character != L'`' && !versionBracket) {
                text.push_back(character);
            }
        }
        text += L"\r\n";
    }
    return text;
}

INT_PTR CALLBACK ChangelogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM) {
    switch (message) {
        case WM_INITDIALOG: {
            const auto markdown = infra::FromUtf8(std::string(EmbeddedChangelog()));
            const std::wstring text = markdown ? ToPlainText(*markdown) : std::wstring();
            SetDlgItemTextW(dialog, IDC_CHANGELOG_TEXT,
                            text.empty() ? L"The changelog is not available." : text.c_str());
            ApplyAppIcon(dialog);
            SetForegroundWindow(dialog);
            return TRUE;
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

void ShowChangelog(HWND owner) {
    DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_CHANGELOG), owner,
                    ChangelogProc, 0);
}

}  // namespace et::ui
