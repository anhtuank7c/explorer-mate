#include "Infrastructure/Utf8.h"

#include <windows.h>

namespace et::infra {

std::string ToUtf8(std::wstring_view text) {
    if (text.empty()) {
        return {};
    }
    const int sourceLength = static_cast<int>(text.size());
    const int size =
        WideCharToMultiByte(CP_UTF8, 0, text.data(), sourceLength, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), sourceLength, utf8.data(), size, nullptr, nullptr);
    return utf8;
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
