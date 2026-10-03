#include "Infrastructure/AppDataPaths.h"

#include <windows.h>

#include <shlobj.h>

#include "Domain/ProductInfo.h"

namespace et::infra {

domain::Result<std::wstring> ProductDataDirectory() {
    PWSTR raw = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &raw);
    std::wstring localAppData = SUCCEEDED(result) ? std::wstring(raw) : std::wstring();
    CoTaskMemFree(raw);
    if (localAppData.empty()) {
        return domain::Error(domain::ErrorCode::OperationFailed,
                             L"Cannot resolve the local application data folder.");
    }
    return localAppData + L"\\" + std::wstring(domain::ProductShortName());
}

}  // namespace et::infra
