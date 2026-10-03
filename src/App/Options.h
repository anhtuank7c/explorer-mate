#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Application/ActionKind.h"
#include "Domain/RenamePlan.h"
#include "Domain/Result.h"

namespace et::ui {

// Command line of ExplorerMate.exe:
//   (no arguments)                         show the introduction window
//   --about, --changelog, --licenses       print that information; no window opens
//   --version
//   --help
//   --request <file>                       run the request written by the shell extension
//   --action <group|rename|duplicate> <item>...   items may be relative to the current folder
// Optional with either form:
//   --silent                               no Shell UI and no undo record
//   --name <folder name>                   answer for "group" instead of asking
//   --mask <name mask> --ext-mask <extension mask>
//   --start <number> --step <number> --digits <number>
//   --search <text> --replace <text>       answer for "rename" instead of asking; any one of
//                                          them is enough, the rest keep their defaults
// Background agent (tray icon + keyboard shortcuts):
//   --agent                                start it (a second one exits immediately)
//   --stop-agent                           ask the running one to exit
//   --autostart <on|off|status>            start the agent when signing in to Windows
// Diagnostics:
//   --diagnose-explorer [--delay <seconds>]  dump Explorer tabs, focus and selections
struct Options {
    bool showVersion = false;
    bool showHelp = false;
    bool showIntroduction = false;  // No arguments: started from the Start menu or by double-click.
    bool showAbout = false;
    bool showChangelog = false;
    bool showLicenses = false;
    bool runAgent = false;
    bool stopAgent = false;
    std::optional<std::wstring> autostart;  // "on", "off" or "status".
    bool diagnoseExplorer = false;
    unsigned delaySeconds = 0;
    unsigned watchSeconds = 0;  // --watch <seconds>: record changes instead of one snapshot.
    bool silent = false;
    std::optional<std::wstring> requestFile;
    std::optional<app::ActionKind> action;
    std::vector<std::wstring> items;
    std::optional<std::wstring> folderName;
    std::optional<domain::RenamePattern> renamePattern;
};

domain::Result<Options> ParseOptions(const std::vector<std::wstring>& arguments);

// The text printed by --help.
std::wstring UsageText();

}  // namespace et::ui
