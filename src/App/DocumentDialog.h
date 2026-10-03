#pragma once

#include <windows.h>

namespace et::ui {

// Read-only windows showing documents that were embedded at build time. `owner` may be null.

// CHANGELOG.md.
void ShowChangelog(HWND owner);
// LICENSE followed by THIRD_PARTY_NOTICES.md.
void ShowLicenses(HWND owner);

}  // namespace et::ui
