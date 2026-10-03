#pragma once

#include <string>
#include <vector>

#include "Application/Ports/IFileOperationGateway.h"
#include "Application/Ports/IFileSystemProbe.h"
#include "Domain/NameCollation.h"
#include "Domain/OperationReport.h"
#include "Domain/Result.h"

namespace et::app {

// Copies every selected file or folder next to itself.
class DuplicateInPlaceUseCase {
public:
    DuplicateInPlaceUseCase(const IFileSystemProbe& probe, IFileOperationGateway& gateway,
                            const domain::INameCollation& collation);

    domain::Result<domain::OperationReport> Execute(std::vector<std::wstring> selectedPaths);

private:
    const IFileSystemProbe& probe_;
    IFileOperationGateway& gateway_;
    const domain::INameCollation& collation_;
};

}  // namespace et::app
