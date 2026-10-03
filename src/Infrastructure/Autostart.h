#pragma once

#include <string>

#include "Domain/Result.h"

namespace et::infra {

// "Start with Windows" for the current user, through the per-user Run registry key. Only
// the value named after the product is ever read, written or deleted.
bool IsAutostartEnabled();

// `commandLine` is what Windows runs at sign-in, e.g. "\"C:\...\ExMate.exe\" --agent".
domain::Status SetAutostart(bool enabled, const std::wstring& commandLine);

}  // namespace et::infra
