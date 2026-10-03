#pragma once

#include <optional>
#include <string>
#include <utility>

#include "Application/Ports/IUserPrompt.h"

namespace et::ui {

// Answers prompts with values given up front (command line). A missing answer counts as
// "cancelled". The use cases re-validate whatever is returned.
class PresetPrompt final : public app::IUserPrompt {
public:
    PresetPrompt(std::optional<std::wstring> folderName,
                 std::optional<domain::RenamePattern> renamePattern)
        : folderName_(std::move(folderName)), renamePattern_(std::move(renamePattern)) {}

    std::optional<std::wstring> AskFolderName(const std::wstring&,
                                              const app::NameValidator&) override {
        return folderName_;
    }

    std::optional<domain::RenamePattern> AskRenamePattern(const domain::RenamePattern&,
                                                          const app::RenamePreviewer&) override {
        return renamePattern_;
    }

private:
    std::optional<std::wstring> folderName_;
    std::optional<domain::RenamePattern> renamePattern_;
};

}  // namespace et::ui
