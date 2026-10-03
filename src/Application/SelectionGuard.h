#pragma once

#include <string>
#include <vector>

#include "Application/Ports/IFileSystemProbe.h"
#include "Domain/NameCollation.h"
#include "Domain/Result.h"
#include "Domain/Selection.h"

namespace et::app {

enum class AllowedItems {
    FilesAndFolders,
    FilesOnly,
};

// Shared precondition of every command: a well-formed selection whose items still exist and
// live in a location the MVP supports (no reparse points, no cloud placeholders).
domain::Result<domain::Selection> ValidateSelection(std::vector<std::wstring> paths,
                                                    AllowedItems allowed,
                                                    const IFileSystemProbe& probe,
                                                    const domain::INameCollation& collation);

}  // namespace et::app
