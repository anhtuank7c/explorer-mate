#pragma once

#include "Application/Ports/ILogger.h"

namespace et::ui {

// Process-wide logger of the shell extension: %LOCALAPPDATA%\ExMate\logs\shell.log.
app::ILogger& ShellLog();

}  // namespace et::ui
