#include "Application/DuplicateInPlaceUseCase.h"

#include <utility>

#include "Application/SelectionGuard.h"

namespace et::app {

DuplicateInPlaceUseCase::DuplicateInPlaceUseCase(const IFileSystemProbe& probe,
                                                 IFileOperationGateway& gateway,
                                                 const domain::INameCollation& collation)
    : probe_(probe), gateway_(gateway), collation_(collation) {}

domain::Result<domain::OperationReport> DuplicateInPlaceUseCase::Execute(
    std::vector<std::wstring> selectedPaths) {
    const auto selection = ValidateSelection(std::move(selectedPaths),
                                             AllowedItems::FilesAndFolders, probe_, collation_);
    if (!selection.ok()) {
        return selection.error();
    }
    return gateway_.DuplicateItems(selection.value().paths());
}

}  // namespace et::app
