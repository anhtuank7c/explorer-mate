#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "Domain/Error.h"

namespace et::domain {

struct NameParts {
    std::wstring stem;
    std::wstring extension;  // Includes the leading dot; empty when there is none.
};

// The extension is the part after the last dot: "archive.tar.gz" -> {"archive.tar", ".gz"}.
// A leading dot is not an extension separator: ".gitignore" -> {".gitignore", ""}.
NameParts SplitStemAndExtension(std::wstring_view name);

// Windows rules for one file or folder name. Returns the reason when the name is not usable.
std::optional<Error> ValidateItemName(std::wstring_view name);

}  // namespace et::domain
