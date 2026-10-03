#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace et::infra {

// For text meant for people (logs, console): characters that cannot be encoded, such as an
// unpaired surrogate, are replaced.
std::string ToUtf8(std::wstring_view text);

// For text that must round-trip exactly (file paths handed to another process): nullopt
// when the text cannot be encoded without loss. NTFS allows unpaired surrogates in names;
// replacing one would silently turn the path into the name of a different file.
std::optional<std::string> ToUtf8Exact(std::wstring_view text);

// nullopt when the bytes are not valid UTF-8.
std::optional<std::wstring> FromUtf8(std::string_view bytes);

}  // namespace et::infra
