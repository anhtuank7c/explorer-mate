#pragma once

#include "Application/Ports/IUserPrompt.h"

namespace et::ui {

// Asks the user through modal Win32 dialogs. Input is validated on every keystroke with the
// callbacks supplied by the use case; OK stays disabled while the input is not acceptable.
class DialogPrompt final : public app::IUserPrompt {
public:
    std::optional<std::wstring> AskFolderName(const std::wstring& suggestion,
                                              const app::NameValidator& validate) override;

    std::optional<domain::RenamePattern> AskRenamePattern(
        const domain::RenamePattern& suggestion, const app::RenamePreviewer& preview) override;
};

}  // namespace et::ui
