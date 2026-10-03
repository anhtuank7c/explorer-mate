#pragma once

#include <string>
#include <vector>

#include "Domain/Result.h"

struct IShellItemArray;

namespace et::infra {

// Filesystem paths of every item, in array order. Fails when any item has no filesystem
// path (virtual namespaces such as Recycle Bin, ZIP contents or MTP devices).
domain::Result<std::vector<std::wstring>> ReadFileSystemPaths(IShellItemArray* items);

}  // namespace et::infra
