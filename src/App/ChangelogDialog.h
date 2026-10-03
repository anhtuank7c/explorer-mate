#pragma once

#include <windows.h>

namespace et::ui {

// Shows the changelog that was embedded at build time. `owner` may be null.
void ShowChangelog(HWND owner);

}  // namespace et::ui
