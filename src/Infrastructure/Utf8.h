#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace et::infra {

std::string ToUtf8(std::wstring_view text);

// nullopt when the bytes are not valid UTF-8.
std::optional<std::wstring> FromUtf8(std::string_view bytes);

}  // namespace et::infra
