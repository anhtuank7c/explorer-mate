#include "Application/BulkRenameUseCase.h"

#include <utility>

#include "Application/SelectionGuard.h"
#include "Domain/PathText.h"

namespace et::app {

BulkRenameUseCase::BulkRenameUseCase(const IFileSystemProbe& probe, IFileOperationGateway& gateway,
                                     IUserPrompt& prompt, const domain::INameCollation& collation)
    : probe_(probe), gateway_(gateway), prompt_(prompt), collation_(collation) {}

domain::Result<domain::OperationReport> BulkRenameUseCase::Execute(
    std::vector<std::wstring> selectedPaths) {
    const auto selection =
        ValidateSelection(std::move(selectedPaths), AllowedItems::FilesOnly, probe_, collation_);
    if (!selection.ok()) {
        return selection.error();
    }
    const std::wstring& parent = selection.value().parent();
    const std::vector<std::wstring> names = selection.value().names();

    // Lists the folder on every call so each preview, and the final plan, see its current state.
    const auto buildPlan =
        [&](const domain::RenamePattern& pattern) -> domain::Result<domain::RenamePlan> {
        const auto siblings = probe_.ListNames(parent);
        if (!siblings.ok()) {
            return siblings.error();
        }
        return domain::BuildRenamePlan(names, siblings.value(), domain::NameOf(parent), pattern,
                                       collation_);
    };

    const RenamePreviewer preview = [&](const domain::RenamePattern& pattern)
        -> domain::Result<std::vector<domain::RenamePreview>> {
        auto plan = buildPlan(pattern);
        if (!plan.ok()) {
            return plan.error();
        }
        return std::move(plan).value().previews;
    };

    const auto pattern = prompt_.AskRenamePattern(domain::RenamePattern{}, preview);
    if (!pattern) {
        return domain::Error(domain::ErrorCode::Cancelled, L"Cancelled.");
    }

    const auto plan = buildPlan(*pattern);
    if (!plan.ok()) {
        return plan.error();
    }
    return gateway_.RenameItems(parent, plan.value().steps);
}

}  // namespace et::app
