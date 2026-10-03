#pragma once

#include <windows.h>

#include <string>

namespace et::ui {

// Read-only windows showing documents that were embedded at build time. `owner` may be null.

// CHANGELOG.md.
void ShowChangelog(HWND owner);
// LICENSE followed by THIRD_PARTY_NOTICES.md.
void ShowLicenses(HWND owner);

// The text those windows show, for the command line. Empty when a document is missing.
std::wstring ChangelogText();
std::wstring LicensesText();

}  // namespace et::ui
