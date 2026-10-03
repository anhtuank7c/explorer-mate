#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "Domain/Error.h"
#include "Domain/Result.h"

namespace et::domain {

// Values a rename mask can refer to for one file.
struct MaskContext {
    std::wstring_view name;       // File name without extension.
    std::wstring_view extension;  // Without the dot; empty when the file has none.
    std::wstring_view counter;    // Already formatted (zero-padded) counter value.
    std::wstring_view parent;     // Name of the folder containing the file.
};

// Mask syntax, modelled on Total Commander's Multi-Rename Tool. Text outside brackets is
// copied as is. Placeholders:
//   [N]      name            [E]      extension
//   [N2-5]   characters 2 to 5          [N2-]  from character 2 to the end
//   [N2]     character 2 only           [N2,3] 3 characters starting at character 2
//            (the same ranges work with E and P)
//   [C]      counter         [P]      parent folder name
//   [[]      a literal "["   []]      a literal "]"
// Positions are 1-based; ranges beyond the end of the text are clipped, not errors.
Result<std::wstring> ExpandMask(std::wstring_view mask, const MaskContext& context);

// The reason the mask cannot be used, or nullopt when it is well-formed.
std::optional<Error> ValidateMask(std::wstring_view mask);

}  // namespace et::domain
