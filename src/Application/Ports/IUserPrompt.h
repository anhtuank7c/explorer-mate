#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Domain/RenamePlan.h"
#include "Domain/Result.h"

namespace et::app {

// Returns the message to show when the name is not acceptable, or nullopt when it is.
using NameValidator = std::function<std::optional<std::wstring>(std::wstring_view)>;

// Returns what the batch would look like with the given pattern, or why it is not possible.
using RenamePreviewer =
    std::function<domain::Result<std::vector<domain::RenamePreview>>(const domain::RenamePattern&)>;

// Questions only the user can answer. nullopt always means the user cancelled.
class IUserPrompt {
public:
    virtual ~IUserPrompt() = default;

    // Must only return a name that `validate` accepts.
    virtual std::optional<std::wstring> AskFolderName(const std::wstring& suggestion,
                                                      const NameValidator& validate) = 0;

    // Must only return a pattern for which `preview` succeeds.
    virtual std::optional<domain::RenamePattern> AskRenamePattern(
        const domain::RenamePattern& suggestion, const RenamePreviewer& preview) = 0;
};

}  // namespace et::app
