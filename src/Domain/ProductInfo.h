#pragma once

#include <string_view>

namespace et::domain {

// "Explorer Mate": what the user sees in window titles, the tray icon and messages.
std::wstring_view ProductName();
// "ExplorerMate": the identifier used where a space would be awkward - the data folder under
// %LOCALAPPDATA%, the autostart registry value, file names.
std::wstring_view ProductShortName();
std::wstring_view ProductVersion();

// Shown in the introduction window. Edit ProductInfo.cpp to change them.
std::wstring_view ProductAuthor();
std::wstring_view ProductWebsiteUrl();    // Full URL opened by the link.
std::wstring_view ProductWebsiteLabel();  // Text the link shows.

}  // namespace et::domain
