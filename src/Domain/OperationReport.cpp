#include "Domain/OperationReport.h"

#include <algorithm>

namespace et::domain {

size_t OperationReport::Count(ItemStatus status) const {
    return static_cast<size_t>(std::count_if(
        items.begin(), items.end(),
        [status](const ItemOutcome& item) { return item.status == status; }));
}

bool OperationReport::AllSucceeded() const {
    return !cancelled && Count(ItemStatus::Succeeded) == items.size();
}

}  // namespace et::domain
