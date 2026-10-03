#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Application/ActionKind.h"
#include "Domain/RenamePlan.h"
#include "Domain/Result.h"

namespace et::ui {

// Command line of ExMate.exe:
//   (no arguments) or --about              show the introduction window
//   --version
//   --request <file>                       run the request written by the shell extension
//   --action <group|rename|duplicate> <item>...
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
// Diagnostics:
//   --diagnose-explorer [--delay <seconds>]  dump Explorer tabs, focus and selections
struct Options {
    bool showVersion = false;
    bool showAbout = false;
    bool runAgent = false;
    bool stopAgent = false;
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

}  // namespace et::ui
