#include "Application/SelectionGuard.h"

#include <optional>
#include <utility>

namespace et::app {

namespace {

using domain::Error;
using domain::ErrorCode;

std::optional<Error> CheckItem(const std::wstring& path, AllowedItems allowed,
                               const IFileSystemProbe& probe) {
    const ItemInfo info = probe.Inspect(path);
    if (!info.exists) {
        return Error(ErrorCode::ItemNotFound, L"This item no longer exists: " + path);
    }
    if (info.isReparsePoint) {
        return Error(ErrorCode::UnsupportedLocation,
                     L"Links and junctions are not supported: " + path);
    }
    if (info.isCloudPlaceholder) {
        return Error(ErrorCode::UnsupportedLocation,
                     L"Online-only cloud files are not supported: " + path);
    }
    if (allowed == AllowedItems::FilesOnly && info.isDirectory) {
        return Error(ErrorCode::InvalidArgument,
                     L"This command works on files only, but a folder is selected: " + path);
    }
    return std::nullopt;
}

}  // namespace

domain::Result<domain::Selection> ValidateSelection(std::vector<std::wstring> paths,
                                                    AllowedItems allowed,
                                                    const IFileSystemProbe& probe,
                                                    const domain::INameCollation& collation) {
    auto selection = domain::Selection::Create(std::move(paths), collation);
    if (!selection.ok()) {
        return selection;
    }
    for (const std::wstring& path : selection.value().paths()) {
        if (const auto problem = CheckItem(path, allowed, probe)) {
            return *problem;
        }
    }
    return selection;
}

}  // namespace et::app
