#include "Domain/PathText.h"

namespace et::domain {

namespace {

constexpr wchar_t kSeparator = L'\\';
constexpr size_t kDriveRootLength = 3;  // "X:\"

bool IsAsciiLetter(wchar_t character) {
    return (character >= L'a' && character <= L'z') || (character >= L'A' && character <= L'Z');
}

}  // namespace

bool IsDriveAbsoluteItemPath(std::wstring_view path) {
    if (path.size() <= kDriveRootLength || !IsAsciiLetter(path[0]) || path[1] != L':' ||
        path[2] != kSeparator) {
        return false;
    }
    if (path.back() == kSeparator || path.find(L'/') != std::wstring_view::npos) {
        return false;
    }
    return path.find(L"\\\\", kDriveRootLength - 1) == std::wstring_view::npos;
}

std::wstring_view ParentOf(std::wstring_view path) {
    const size_t separator = path.find_last_of(kSeparator);
    if (separator == std::wstring_view::npos) {
        return {};
    }
    const bool parentIsDriveRoot = separator == kDriveRootLength - 1;
    return path.substr(0, parentIsDriveRoot ? kDriveRootLength : separator);
}

std::wstring_view NameOf(std::wstring_view path) {
    const size_t separator = path.find_last_of(kSeparator);
    return separator == std::wstring_view::npos ? path : path.substr(separator + 1);
}

std::wstring JoinPath(std::wstring_view parent, std::wstring_view name) {
    std::wstring joined(parent);
    if (!joined.empty() && joined.back() != kSeparator) {
        joined.push_back(kSeparator);
    }
    joined.append(name);
    return joined;
}

}  // namespace et::domain
