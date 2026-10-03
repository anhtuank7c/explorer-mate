#pragma once

#include "Application/Ports/ISelectionSource.h"

namespace et::infra {

// Reads the selection of the File Explorer tab the user is working in. Used by keyboard
// shortcuts, which - unlike the context menu - are not handed a selection by Explorer.
//
// The focused tab is identified by three facts that must all agree:
//   1. the foreground window is an Explorer frame,
//   2. exactly one tab of that frame is the shown one,
//   3. the keyboard focus is the file list of that very tab (not a rename box, the address
//      bar, the search box or the navigation pane).
// Anything else is refused. CaptureFocusedSelection needs a COM apartment.
class ExplorerSelectionSource final : public app::ISelectionSource {
public:
    bool FocusIsInFileList() const override;
    domain::Result<std::vector<std::wstring>> CaptureFocusedSelection() const override;
};

}  // namespace et::infra
