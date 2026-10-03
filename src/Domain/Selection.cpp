#include "Domain/Selection.h"

#include <utility>

#include "Domain/PathText.h"

namespace et::domain {

Selection::Selection(std::wstring parent, std::vector<std::wstring> paths)
    : parent_(std::move(parent)), paths_(std::move(paths)) {}

Result<Selection> Selection::Create(std::vector<std::wstring> paths,
                                    const INameCollation& collation) {
    if (paths.empty()) {
        return Error(ErrorCode::EmptySelection, L"Nothing is selected.");
    }
    for (const std::wstring& path : paths) {
        if (!IsDriveAbsoluteItemPath(path)) {
            return Error(ErrorCode::UnsupportedLocation,
                         L"Only items on a local drive are supported: " + path);
        }
    }

    const std::wstring parent(ParentOf(paths.front()));
    for (size_t index = 0; index < paths.size(); ++index) {
        if (!collation.Equals(ParentOf(paths[index]), parent)) {
            return Error(ErrorCode::MixedParents,
                         L"All selected items must be in the same folder.");
        }
        for (size_t earlier = 0; earlier < index; ++earlier) {
            if (collation.Equals(NameOf(paths[earlier]), NameOf(paths[index]))) {
                return Error(ErrorCode::InvalidArgument,
                             L"The selection contains the same item twice: " + paths[index]);
            }
        }
    }
    return Selection(parent, std::move(paths));
}

std::vector<std::wstring> Selection::names() const {
    std::vector<std::wstring> names;
    names.reserve(paths_.size());
    for (const std::wstring& path : paths_) {
        names.emplace_back(NameOf(path));
    }
    return names;
}

}  // namespace et::domain
