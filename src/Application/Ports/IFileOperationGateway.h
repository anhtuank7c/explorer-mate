#pragma once

#include <string>
#include <vector>

#include "Domain/OperationReport.h"
#include "Domain/RenamePlan.h"
#include "Domain/Result.h"

namespace et::app {

// Everything that changes the filesystem. Implementations never overwrite or merge.
class IFileOperationGateway {
public:
    virtual ~IFileOperationGateway() = default;

    // Fails with NameCollision when anything already exists at `path`, so a success proves
    // that this call created the folder.
    virtual domain::Status CreateNewFolder(const std::wstring& path) = 0;

    // Removes `path` only if it is an empty folder. An occupied folder is left untouched.
    virtual domain::Status RemoveFolderIfEmpty(const std::wstring& path) = 0;

    virtual domain::OperationReport MoveItemsInto(const std::vector<std::wstring>& sources,
                                                  const std::wstring& destinationFolder) = 0;

    // Copies each source next to itself under a new, non-colliding name chosen by the
    // platform. The report carries the actual destination of every copy.
    virtual domain::OperationReport DuplicateItems(const std::vector<std::wstring>& sources) = 0;

    // Runs the steps in order inside `folder`. Steps depend on each other, so the first
    // failure stops the batch and the remaining steps are reported as NotAttempted.
    virtual domain::OperationReport RenameItems(const std::wstring& folder,
                                                const std::vector<domain::RenameStep>& steps) = 0;
};

}  // namespace et::app
