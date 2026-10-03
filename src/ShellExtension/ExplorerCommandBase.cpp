#include "ShellExtension/ExplorerCommandBase.h"

#include <shlwapi.h>
#include <wrl/client.h>

#include <string>

#include "Infrastructure/ShellSelection.h"
#include "ShellExtension/ShellLog.h"
#include "ShellExtension/WorkerLauncher.h"

namespace et::ui {

namespace {

using Microsoft::WRL::ComPtr;

// GetState must stay cheap: beyond this many items the command is simply shown and the
// worker's full validation decides.
constexpr DWORD kMaxItemsInspected = 256;

bool IsFolder(IShellItem* item) {
    // ZIP archives also report SFGAO_FOLDER; real folders are the ones that are not streams.
    SFGAOF attributes = 0;
    return SUCCEEDED(item->GetAttributes(SFGAO_FOLDER | SFGAO_STREAM, &attributes)) &&
           (attributes & SFGAO_FOLDER) != 0 && (attributes & SFGAO_STREAM) == 0;
}

bool ContainsFolder(IShellItemArray* items) {
    DWORD count = 0;
    if (items == nullptr || FAILED(items->GetCount(&count)) || count > kMaxItemsInspected) {
        return false;
    }
    for (DWORD index = 0; index < count; ++index) {
        ComPtr<IShellItem> item;
        if (SUCCEEDED(items->GetItemAt(index, &item)) && IsFolder(item.Get())) {
            return true;
        }
    }
    return false;
}

}  // namespace

IFACEMETHODIMP ExplorerCommandBase::GetTitle(IShellItemArray*, PWSTR* name) {
    if (name == nullptr) {
        return E_POINTER;
    }
    *name = nullptr;
    try {
        return SHStrDupW(std::wstring(Title()).c_str(), name);
    } catch (...) {
        return E_OUTOFMEMORY;
    }
}

IFACEMETHODIMP ExplorerCommandBase::GetIcon(IShellItemArray*, PWSTR* icon) {
    if (icon != nullptr) {
        *icon = nullptr;
    }
    return E_NOTIMPL;
}

IFACEMETHODIMP ExplorerCommandBase::GetToolTip(IShellItemArray*, PWSTR* tip) {
    if (tip != nullptr) {
        *tip = nullptr;
    }
    return E_NOTIMPL;
}

IFACEMETHODIMP ExplorerCommandBase::GetCanonicalName(GUID* name) {
    if (name == nullptr) {
        return E_POINTER;
    }
    *name = GUID_NULL;
    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::GetState(IShellItemArray* items, BOOL, EXPCMDSTATE* state) {
    if (state == nullptr) {
        return E_POINTER;
    }
    *state = (!AcceptsFolders() && ContainsFolder(items)) ? ECS_HIDDEN : ECS_ENABLED;
    return S_OK;
}

// No exception may cross the COM boundary into the shell host.
IFACEMETHODIMP ExplorerCommandBase::Invoke(IShellItemArray* items, IBindCtx*) {
    try {
        auto paths = infra::ReadFileSystemPaths(items);
        if (!paths.ok()) {
            ShellLog().Write(app::LogLevel::Warning, paths.error().message);
            return S_OK;
        }
        const auto launched = LaunchWorker({Action(), std::move(paths).value()});
        if (!launched.ok()) {
            ShellLog().Write(app::LogLevel::Error, launched.error().message);
        }
        return S_OK;
    } catch (...) {
        return E_FAIL;
    }
}

IFACEMETHODIMP ExplorerCommandBase::GetFlags(EXPCMDFLAGS* flags) {
    if (flags == nullptr) {
        return E_POINTER;
    }
    *flags = ECF_DEFAULT;
    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::EnumSubCommands(IEnumExplorerCommand** commands) {
    if (commands != nullptr) {
        *commands = nullptr;
    }
    return E_NOTIMPL;
}

}  // namespace et::ui
