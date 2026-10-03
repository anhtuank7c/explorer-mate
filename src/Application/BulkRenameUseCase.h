#pragma once

#include <string>
#include <vector>

#include "Application/Ports/IFileOperationGateway.h"
#include "Application/Ports/IFileSystemProbe.h"
#include "Application/Ports/IUserPrompt.h"
#include "Domain/NameCollation.h"
#include "Domain/OperationReport.h"
#include "Domain/Result.h"

namespace et::app {

// Renames the selected files according to a rename mask chosen by the user.
class BulkRenameUseCase {
public:
    BulkRenameUseCase(const IFileSystemProbe& probe, IFileOperationGateway& gateway,
                      IUserPrompt& prompt, const domain::INameCollation& collation);

    // Fails with ErrorCode::Cancelled, without touching anything, when the user cancels.
    domain::Result<domain::OperationReport> Execute(std::vector<std::wstring> selectedPaths);

private:
    const IFileSystemProbe& probe_;
    IFileOperationGateway& gateway_;
    IUserPrompt& prompt_;
    const domain::INameCollation& collation_;
};

}  // namespace et::app
