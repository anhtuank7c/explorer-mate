#pragma once

#include <string>
#include <string_view>

#include "Domain/Result.h"

namespace et::domain {

// A folder name that is guaranteed to satisfy the Windows naming rules.
class FolderName {
public:
    static Result<FolderName> Create(std::wstring_view name);

    const std::wstring& value() const { return value_; }

private:
    explicit FolderName(std::wstring value);

    std::wstring value_;
};

}  // namespace et::domain
