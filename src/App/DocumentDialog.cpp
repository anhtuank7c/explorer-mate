#include "App/DocumentDialog.h"

#include <string>
#include <string_view>

#include "App/AppIcon.h"
#include "App/resource.h"
#include "Infrastructure/Utf8.h"

namespace et::ui {

namespace {

struct Document {
    const wchar_t* caption;
    std::wstring text;
};

// The text of an embedded UTF-8 file, or empty when the resource is missing or damaged.
std::wstring EmbeddedText(int resourceId) {
    const HMODULE module = GetModuleHandleW(nullptr);
    const HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    const HGLOBAL loaded = resource != nullptr ? LoadResource(module, resource) : nullptr;
    const void* bytes = loaded != nullptr ? LockResource(loaded) : nullptr;
    if (bytes == nullptr) {
        return {};
    }
    const auto text = infra::FromUtf8(
        std::string_view(static_cast<const char*>(bytes), SizeofResource(module, resource)));
    return text ? *text : std::wstring();
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

enum class StartAt { Beginning, FirstVersionHeading };

// Turns Markdown into plain text for an edit control: drops heading marks, code fences and
// code ticks, the brackets around versions, and shows list items with a bullet. Plain text
// passes through with only its line ends changed.
std::wstring ToPlainText(std::wstring_view markdown, StartAt startAt) {
    std::wstring text;
    bool started = startAt == StartAt::Beginning;
    while (!markdown.empty()) {
        std::wstring_view line = TakeLine(markdown);
        const bool versionHeading = line.substr(0, 3) == L"## ";
        started = started || versionHeading;
        if (!started || line.substr(0, 3) == L"```") {
            continue;
        }
        if (line.substr(0, 2) == L"- ") {
            text += L"   \x2022  ";
            line.remove_prefix(2);
        } else if (!line.empty() && line.front() == L'#') {
            const size_t content = line.find_first_not_of(L"# ");
            line.remove_prefix(content == std::wstring_view::npos ? line.size() : content);
        }
        for (const wchar_t character : line) {
            const bool versionBracket = startAt == StartAt::FirstVersionHeading && versionHeading &&
                                        (character == L'[' || character == L']');
            if (character != L'`' && !versionBracket) {
                text.push_back(character);
            }
        }
        text += L"\r\n";
    }
    return text;
}

INT_PTR CALLBACK DocumentProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            const auto* document = reinterpret_cast<const Document*>(lParam);
            SetWindowTextW(dialog, document->caption);
            SetDlgItemTextW(dialog, IDC_DOCUMENT_TEXT,
                            document->text.empty() ? L"This document is not available."
                                                   : document->text.c_str());
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

void ShowDocument(HWND owner, const Document& document) {
    DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_DOCUMENT), owner, DocumentProc,
                    reinterpret_cast<LPARAM>(&document));
}

}  // namespace

std::wstring ChangelogText() {
    return ToPlainText(EmbeddedText(IDR_CHANGELOG), StartAt::FirstVersionHeading);
}

std::wstring LicensesText() {
    const std::wstring license = ToPlainText(EmbeddedText(IDR_LICENSE), StartAt::Beginning);
    const std::wstring notices = ToPlainText(EmbeddedText(IDR_NOTICES), StartAt::Beginning);
    if (license.empty() || notices.empty()) {
        return {};
    }
    return L"Explorer Mate\r\n\r\n" + license +
           L"\r\n________________________________________\r\n\r\n" + notices;
}

void ShowChangelog(HWND owner) {
    ShowDocument(owner, {L"What's new in Explorer Mate", ChangelogText()});
}

void ShowLicenses(HWND owner) {
    ShowDocument(owner, {L"Explorer Mate licenses", LicensesText()});
}

}  // namespace et::ui
