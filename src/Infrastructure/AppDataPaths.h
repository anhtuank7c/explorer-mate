#pragma once

#include <string>

#include "Domain/Result.h"

namespace et::infra {

// %LOCALAPPDATA%\ExplorerMate, without a trailing separator. The directory is not created.
domain::Result<std::wstring> ProductDataDirectory();

}  // namespace et::infra
