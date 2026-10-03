#pragma once

#include <string_view>

#include "Domain/Result.h"

namespace et::app {

enum class ActionKind {
    GroupIntoNewFolder,
    BulkRename,
    DuplicateInPlace,
};

// Wire names shared by the request file and the command line.
std::wstring_view ToWireName(ActionKind action);
domain::Result<ActionKind> ParseActionKind(std::wstring_view wireName);

}  // namespace et::app
