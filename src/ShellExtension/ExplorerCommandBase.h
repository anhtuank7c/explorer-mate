#pragma once

#include <windows.h>

#include <shobjidl_core.h>
#include <wrl/implements.h>

#include <string_view>

#include "Application/ActionKind.h"

namespace et::ui {

// Shared IExplorerCommand plumbing. Explorer calls GetTitle/GetState while building the menu,
// so nothing here may scan folders or block. Invoke only hands the selection to the worker
// process; dialogs and file operations never run inside the shell host.
class ExplorerCommandBase
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IExplorerCommand> {
public:
    IFACEMETHODIMP GetTitle(IShellItemArray* items, PWSTR* name) override;
    IFACEMETHODIMP GetIcon(IShellItemArray* items, PWSTR* icon) override;
    IFACEMETHODIMP GetToolTip(IShellItemArray* items, PWSTR* tip) override;
    IFACEMETHODIMP GetCanonicalName(GUID* name) override;
    IFACEMETHODIMP GetState(IShellItemArray* items, BOOL okToBeSlow, EXPCMDSTATE* state) override;
    IFACEMETHODIMP Invoke(IShellItemArray* items, IBindCtx* context) override;
    IFACEMETHODIMP GetFlags(EXPCMDFLAGS* flags) override;
    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** commands) override;

protected:
    virtual std::wstring_view Title() const = 0;
    virtual app::ActionKind Action() const = 0;
    // False hides the command when the selection contains a folder.
    virtual bool AcceptsFolders() const { return true; }
};

}  // namespace et::ui
