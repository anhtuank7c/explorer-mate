#pragma once

#include <string>

#include "Domain/Result.h"

namespace et::infra {

// "Start with Windows" for the current user. Installed from a package, this is the startup
// task the package declares (Windows lists it under Settings > Apps > Startup). Otherwise it
// is the per-user Run registry key, where only the value named after the product is ever
// read, written or deleted. The calling thread must be in a COM single-threaded apartment.
bool IsAutostartEnabled();

// `commandLine` is what Windows runs at sign-in, e.g. "\"C:\...\ExplorerMate.exe\" --agent";
// the package's startup task has its own and ignores it. Fails with a message for the user
// when Windows does not allow the task to be turned on.
domain::Status SetAutostart(bool enabled, const std::wstring& commandLine);

}  // namespace et::infra
