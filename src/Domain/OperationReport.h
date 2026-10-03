#pragma once

#include <string>
#include <vector>

namespace et::domain {

enum class ItemStatus {
    Succeeded,
    Failed,
    Skipped,
    NotAttempted,
};

struct ItemOutcome {
    std::wstring source;
    std::wstring destination;  // Actual resulting path; empty unless the item succeeded.
    ItemStatus status = ItemStatus::NotAttempted;
    std::wstring detail;       // Human-readable reason for Failed/Skipped.
};

// Shell operations are not transactions: a batch can end partly done, so the result is
// reported per item, separately from whether the user cancelled the batch.
struct OperationReport {
    std::vector<ItemOutcome> items;
    bool cancelled = false;

    size_t Count(ItemStatus status) const;
    bool AllSucceeded() const;
};

}  // namespace et::domain
