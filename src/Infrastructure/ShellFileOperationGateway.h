#pragma once

#include "Application/Ports/IFileOperationGateway.h"

namespace et::infra {

enum class OperationUi {
    Interactive,  // Shell progress, error and conflict dialogs; operations can be undone.
    Silent,       // No UI and no undo record; anything that would need a dialog fails instead.
};

// Moves, copies and renames through IFileOperation so the user gets the standard progress
// UI and Undo (Ctrl+Z) entries. The calling thread must be in a COM single-threaded apartment.
//
// IFileOperation is not transactional: every method reports per-item outcomes collected
// from the progress sink instead of a single success flag.
class ShellFileOperationGateway final : public app::IFileOperationGateway {
public:
    explicit ShellFileOperationGateway(OperationUi ui);

    domain::Status CreateNewFolder(const std::wstring& path) override;
    domain::Status RemoveFolderIfEmpty(const std::wstring& path) override;
    domain::OperationReport MoveItemsInto(const std::vector<std::wstring>& sources,
                                          const std::wstring& destinationFolder) override;
    domain::OperationReport DuplicateItems(const std::vector<std::wstring>& sources) override;
    domain::OperationReport RenameItems(const std::wstring& folder,
                                        const std::vector<domain::RenameStep>& steps) override;

private:
    OperationUi ui_;
};

}  // namespace et::infra
