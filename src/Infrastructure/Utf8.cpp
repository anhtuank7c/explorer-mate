#include "Infrastructure/Utf8.h"

#include <windows.h>

namespace et::infra {

namespace {

// `flags` is 0 (replace what cannot be encoded) or WC_ERR_INVALID_CHARS (fail instead).
std::optional<std::string> Encode(std::wstring_view text, DWORD flags) {
    if (text.empty()) {
        return std::string();
    }
    const int sourceLength = static_cast<int>(text.size());
    const int size = WideCharToMultiByte(CP_UTF8, flags, text.data(), sourceLength, nullptr, 0,
                                         nullptr, nullptr);
    if (size <= 0) {
        return std::nullopt;
    }
    std::string utf8(static_cast<size_t>(size), '\0');
    if (WideCharToMultiByte(CP_UTF8, flags, text.data(), sourceLength, utf8.data(), size, nullptr,
                            nullptr) != size) {
        return std::nullopt;
    }
    return utf8;
}

}  // namespace

std::string ToUtf8(std::wstring_view text) {
    return Encode(text, 0).value_or(std::string());
}

std::optional<std::string> ToUtf8Exact(std::wstring_view text) {
    return Encode(text, WC_ERR_INVALID_CHARS);
}

std::optional<std::wstring> FromUtf8(std::string_view bytes) {
    if (bytes.empty()) {
        return std::wstring();
    }
    const int sourceLength = static_cast<int>(bytes.size());
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), sourceLength,
                                         nullptr, 0);
    if (size <= 0) {
        return std::nullopt;
    }
    std::wstring text(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), sourceLength, text.data(), size);
    return text;
}

}  // namespace et::infra
