#include "Infrastructure/ErrorText.h"

#include <windows.h>

#include <cwchar>

namespace et::infra {

namespace {

std::wstring SystemMessage(DWORD code) {
    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    std::wstring message(buffer != nullptr ? buffer : L"", length);
    LocalFree(buffer);
    while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n' ||
                                message.back() == L' ')) {
        message.pop_back();
    }
    return message;
}

std::wstring HexCode(DWORD code) {
    wchar_t text[16]{};
    swprintf_s(text, L"0x%08X", code);
    return text;
}

}  // namespace

std::wstring DescribeHresult(long result) {
    const DWORD code = static_cast<DWORD>(result);
    std::wstring message = SystemMessage(code);
    if (message.empty()) {
        message = L"The operation failed.";
    }
    return message + L" (" + HexCode(code) + L")";
}

std::wstring DescribeWin32Error(unsigned long error) {
    return DescribeHresult(HRESULT_FROM_WIN32(error));
}

}  // namespace et::infra
