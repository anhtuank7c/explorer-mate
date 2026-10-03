#pragma once

#include <string>
#include <vector>

#include "Domain/Result.h"

namespace et::app {

// Where a keyboard shortcut gets its items from: the file list the user is looking at.
class ISelectionSource {
public:
    virtual ~ISelectionSource() = default;

    // Cheap yes/no for the keyboard hook: is the focus in a file list right now?
    virtual bool FocusIsInFileList() const = 0;

    // Paths selected in the focused file list. Must fail rather than guess when the focused
    // list cannot be identified unambiguously (several candidates, focus moved, virtual
    // folder): acting on the wrong selection moves or renames the wrong files.
    virtual domain::Result<std::vector<std::wstring>> CaptureFocusedSelection() const = 0;
};

}  // namespace et::app
