#include "Domain/PathText.h"

namespace et::domain {

namespace {

constexpr wchar_t kSeparator = L'\\';
constexpr size_t kDriveRootLength = 3;  // "X:\"

bool IsAsciiLetter(wchar_t character) {
    return (character >= L'a' && character <= L'z') || (character >= L'A' && character <= L'Z');
}

// One path component as it must look for the text of a path to mean exactly one item:
// Windows silently normalises "." / "..", trailing dots and spaces, so two different texts
// could name the same file, and ':' would address an alternate data stream.
bool IsPlainComponent(std::wstring_view component) {
    if (component.empty() || component == L"." || component == L"..") {
        return false;
    }
    if (component.back() == L'.' || component.back() == L' ') {
        return false;
    }
    for (const wchar_t character : component) {
        if (character < 0x20 || character == L':' || character == L'/') {
            return false;
        }
    }
    return true;
}

}  // namespace

bool IsDriveAbsoluteItemPath(std::wstring_view path) {
    if (path.size() <= kDriveRootLength || !IsAsciiLetter(path[0]) || path[1] != L':' ||
        path[2] != kSeparator) {
        return false;
    }
    std::wstring_view rest = path.substr(kDriveRootLength);
    while (true) {
        const size_t separator = rest.find(kSeparator);
        if (!IsPlainComponent(rest.substr(0, separator))) {
            return false;  // Also catches empty components and a trailing separator.
        }
        if (separator == std::wstring_view::npos) {
            return true;
        }
        rest.remove_prefix(separator + 1);
    }
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
