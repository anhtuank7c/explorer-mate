#include "Infrastructure/ShellSelection.h"

#include <windows.h>

#include <shobjidl_core.h>
#include <wrl/client.h>

namespace et::infra {

namespace {

using Microsoft::WRL::ComPtr;

domain::Error SelectionError(domain::ErrorCode code, const wchar_t* message) {
    return domain::Error(code, message);
}

}  // namespace

domain::Result<std::vector<std::wstring>> ReadFileSystemPaths(IShellItemArray* items) {
    if (items == nullptr) {
        return SelectionError(domain::ErrorCode::EmptySelection, L"No items were selected.");
    }
    DWORD count = 0;
    if (FAILED(items->GetCount(&count))) {
        return SelectionError(domain::ErrorCode::OperationFailed, L"Cannot read the selection.");
    }

    std::vector<std::wstring> paths;
    paths.reserve(count);
    for (DWORD index = 0; index < count; ++index) {
        ComPtr<IShellItem> item;
        if (FAILED(items->GetItemAt(index, &item))) {
            return SelectionError(domain::ErrorCode::OperationFailed, L"Cannot read the selection.");
        }
        PWSTR raw = nullptr;
        if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw))) {
            return SelectionError(domain::ErrorCode::UnsupportedLocation,
                                  L"The selection contains an item that is not a file or folder.");
        }
        paths.emplace_back(raw);
        CoTaskMemFree(raw);
    }
    return paths;
}

}  // namespace et::infra
