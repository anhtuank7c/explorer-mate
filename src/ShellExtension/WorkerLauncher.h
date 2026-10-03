#pragma once

#include "Application/ActionRequest.h"
#include "Domain/Result.h"

namespace et::ui {

// Writes the request to the per-user request folder and starts ExplorerMate.exe (located
// next to this DLL) to carry it out. Returns as soon as the process is started: the shell
// host must never wait on dialogs or file operations.
domain::Status LaunchWorker(const app::ActionRequest& request);

}  // namespace et::ui
