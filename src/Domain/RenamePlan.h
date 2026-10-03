#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Domain/NameCollation.h"
#include "Domain/Result.h"

namespace et::domain {

// How a batch is renamed, modelled on Total Commander's Multi-Rename Tool:
//   new name = expand(nameMask) + "." + expand(extensionMask), then search/replace.
// See RenameMask.h for the mask syntax. The default appends "_" and a running number.
struct RenamePattern {
    std::wstring nameMask = L"[N]_[C]";
    std::wstring extensionMask = L"[E]";
    unsigned counterStart = 1;
    unsigned counterStep = 1;
    // Minimum width of [C]. It grows automatically so every counter of the batch has the
    // same number of digits and the results sort correctly.
    unsigned counterDigits = 2;
    std::wstring searchFor;    // Empty: no replacement. Case-sensitive, every occurrence.
    std::wstring replaceWith;
};

struct RenamePreview {
    std::wstring original;
    std::wstring renamed;
};

struct RenameStep {
    std::wstring from;
    std::wstring to;
};

struct RenamePlan {
    // One entry per selected file, in numbering order. What the user sees.
    std::vector<RenamePreview> previews;
    // Renames to run in this exact order so that every target is free when its step runs.
    // May contain extra hops through temporary names when the targets form a cycle.
    std::vector<RenameStep> steps;
};

// Computes the whole batch up front. Fails, before anything is renamed, when a new name is
// invalid or would collide with an item of `siblingNames` that is not part of the selection.
// `siblingNames` is every name currently in the parent folder (selected ones included).
// `parentName` is the name of that folder, used by the [P] placeholder.
Result<RenamePlan> BuildRenamePlan(std::vector<std::wstring> selectedNames,
                                   const std::vector<std::wstring>& siblingNames,
                                   std::wstring_view parentName,
                                   const RenamePattern& pattern,
                                   const INameCollation& collation);

}  // namespace et::domain
