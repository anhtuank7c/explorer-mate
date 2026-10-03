#pragma once

#include <string>
#include <vector>

#include "Domain/Result.h"

namespace et::app {

struct ItemInfo {
    bool exists = false;
    bool isDirectory = false;
    bool isReparsePoint = false;     // Symbolic link, junction or mount point.
    bool isCloudPlaceholder = false; // Not fully present on disk (e.g. OneDrive online-only).
};

// Read-only questions about the filesystem.
class IFileSystemProbe {
public:
    virtual ~IFileSystemProbe() = default;

    virtual ItemInfo Inspect(const std::wstring& path) const = 0;

    // Names of every entry directly inside `folder`.
    virtual domain::Result<std::vector<std::wstring>> ListNames(const std::wstring& folder) const = 0;
};

}  // namespace et::app
