#include "Infrastructure/WindowsNameCollation.h"

#include <windows.h>

#include <shlwapi.h>

#include <string>

// Every binary that links this static library needs shlwapi for StrCmpLogicalW.
#pragma comment(lib, "shlwapi.lib")

namespace et::infra {

bool WindowsNameCollation::Equals(std::wstring_view left, std::wstring_view right) const {
    return CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(),
                                static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

bool WindowsNameCollation::NaturalLess(std::wstring_view left, std::wstring_view right) const {
    const std::wstring leftText(left);
    const std::wstring rightText(right);
    const int order = StrCmpLogicalW(leftText.c_str(), rightText.c_str());
    // Names that tie logically ("a01" vs "a1") still need a stable, strict order.
    return order != 0 ? order < 0 : leftText < rightText;
}

}  // namespace et::infra
