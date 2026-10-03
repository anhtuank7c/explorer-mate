#include "Domain/FolderName.h"

#include <utility>

#include "Domain/ItemName.h"

namespace et::domain {

FolderName::FolderName(std::wstring value) : value_(std::move(value)) {}

Result<FolderName> FolderName::Create(std::wstring_view name) {
    if (const auto problem = ValidateItemName(name)) {
        return *problem;
    }
    return FolderName(std::wstring(name));
}

}  // namespace et::domain
