#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Application/ActionKind.h"
#include "Domain/Result.h"

namespace et::app {

// What the shell extension hands to the worker: which command, on which items.
struct ActionRequest {
    ActionKind action = ActionKind::DuplicateInPlace;
    std::vector<std::wstring> items;
};

// Line-based text format (file names cannot contain line breaks on Windows):
//   ExMate-Request 1
//   action=<wire name>
//   item=<absolute path>      (one line per item)
std::wstring SerializeRequest(const ActionRequest& request);

// Treats the text as untrusted: unknown versions, keys, actions or malformed lines fail.
// Paths are only checked for shape here; the use cases validate them against the filesystem.
domain::Result<ActionRequest> ParseRequest(std::wstring_view text);

}  // namespace et::app
