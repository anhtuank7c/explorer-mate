#pragma once

#include <windows.h>

namespace et::ui {

// The application icon at the system's small-icon size (title bars, tray). Shared: the
// caller must not destroy it.
HICON SmallAppIcon();

// Gives a top-level window or dialog the application icon in its title bar and taskbar
// button, instead of the generic Windows one.
void ApplyAppIcon(HWND window);

}  // namespace et::ui
