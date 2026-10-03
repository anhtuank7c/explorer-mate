#pragma once

#include <string>
#include <vector>

#include "Domain/NameCollation.h"
#include "Domain/Result.h"

namespace et::domain {

// The items a command acts on: at least one drive-absolute path, all in the same parent
// folder, without duplicates. Built from text only; existence is checked by the use cases.
class Selection {
public:
    static Result<Selection> Create(std::vector<std::wstring> paths, const INameCollation& collation);

    const std::wstring& parent() const { return parent_; }
    const std::vector<std::wstring>& paths() const { return paths_; }
    std::vector<std::wstring> names() const;

private:
    Selection(std::wstring parent, std::vector<std::wstring> paths);

    std::wstring parent_;
    std::vector<std::wstring> paths_;
};

}  // namespace et::domain
