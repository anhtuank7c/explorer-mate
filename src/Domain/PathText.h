#pragma once

#include <string>
#include <string_view>

namespace et::domain {

// Pure text helpers for drive-absolute Windows paths ("D:\Work\a.txt"). No filesystem access.

// True for "X:\name[\name...]" where every component is a plain name: no empty, "." or ".."
// components, no trailing separator, dot or space, no control characters, '/' or ':'.
// Such a text names exactly one item, so comparing texts is comparing items.
bool IsDriveAbsoluteItemPath(std::wstring_view path);

// "D:\Work\a.txt" -> "D:\Work"; "D:\a.txt" -> "D:\".
std::wstring_view ParentOf(std::wstring_view path);

// "D:\Work\a.txt" -> "a.txt".
std::wstring_view NameOf(std::wstring_view path);

std::wstring JoinPath(std::wstring_view parent, std::wstring_view name);

}  // namespace et::domain
