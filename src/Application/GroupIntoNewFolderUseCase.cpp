#include "Application/GroupIntoNewFolderUseCase.h"

#include <algorithm>
#include <utility>

#include "Application/SelectionGuard.h"
#include "Domain/FolderName.h"
#include "Domain/PathText.h"

namespace et::app {

namespace {

constexpr const wchar_t* kDefaultFolderName = L"New Folder";

bool IsTaken(const std::vector<std::wstring>& siblingNames, std::wstring_view name,
             const domain::INameCollation& collation) {
    return std::any_of(siblingNames.begin(), siblingNames.end(), [&](const std::wstring& sibling) {
        return collation.Equals(sibling, name);
    });
}

// "New Folder", then "New Folder (2)", "New Folder (3)"... like Explorer does.
std::wstring SuggestFreeName(const std::vector<std::wstring>& siblingNames,
                             const domain::INameCollation& collation) {
    std::wstring candidate = kDefaultFolderName;
    for (unsigned counter = 2; IsTaken(siblingNames, candidate, collation); ++counter) {
        candidate = std::wstring(kDefaultFolderName) + L" (" + std::to_wstring(counter) + L")";
    }
    return candidate;
}

}  // namespace

GroupIntoNewFolderUseCase::GroupIntoNewFolderUseCase(const IFileSystemProbe& probe,
                                                     IFileOperationGateway& gateway,
                                                     IUserPrompt& prompt,
                                                     const domain::INameCollation& collation)
    : probe_(probe), gateway_(gateway), prompt_(prompt), collation_(collation) {}

domain::Result<domain::OperationReport> GroupIntoNewFolderUseCase::Execute(
    std::vector<std::wstring> selectedPaths) {
    const auto selection = ValidateSelection(std::move(selectedPaths),
                                             AllowedItems::FilesAndFolders, probe_, collation_);
    if (!selection.ok()) {
        return selection.error();
    }
    const std::wstring& parent = selection.value().parent();

    const auto siblings = probe_.ListNames(parent);
    if (!siblings.ok()) {
        return siblings.error();
    }

    const NameValidator validate = [&](std::wstring_view name) -> std::optional<std::wstring> {
        const auto folderName = domain::FolderName::Create(name);
        if (!folderName.ok()) {
            return folderName.error().message;
        }
        if (IsTaken(siblings.value(), name, collation_)) {
            return std::wstring(L"An item with this name already exists in this folder.");
        }
        return std::nullopt;
    };

    const auto chosenName =
        prompt_.AskFolderName(SuggestFreeName(siblings.value(), collation_), validate);
    if (!chosenName) {
        return domain::Error(domain::ErrorCode::Cancelled, L"Cancelled.");
    }
    // The prompt is an outer layer: never trust it to have applied the validator.
    if (const auto problem = validate(*chosenName)) {
        return domain::Error(domain::ErrorCode::InvalidName, *problem);
    }

    // Only a folder this request created may receive the items. If something appeared under
    // that name since the listing, creation fails and nothing is moved.
    const std::wstring destination = domain::JoinPath(parent, *chosenName);
    if (const auto created = gateway_.CreateNewFolder(destination); !created.ok()) {
        return created.error();
    }

    domain::OperationReport report = gateway_.MoveItemsInto(selection.value().paths(), destination);
    if (report.Count(domain::ItemStatus::Succeeded) == 0) {
        // Best effort: do not leave an empty folder behind. Never deletes a non-empty folder.
        (void)gateway_.RemoveFolderIfEmpty(destination);
    }
    return report;
}

}  // namespace et::app
