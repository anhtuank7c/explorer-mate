#include "Infrastructure/Autostart.h"

#include <windows.h>

#include "Domain/ProductInfo.h"
#include "Infrastructure/ErrorText.h"

namespace et::infra {

namespace {

constexpr const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

std::wstring ValueName() {
    return std::wstring(domain::ProductShortName());
}

domain::Error RegistryError(LSTATUS status) {
    return domain::Error(domain::ErrorCode::OperationFailed,
                         L"Cannot change the startup setting: " +
                             DescribeWin32Error(static_cast<unsigned long>(status)));
}

}  // namespace

bool IsAutostartEnabled() {
    return RegGetValueW(HKEY_CURRENT_USER, kRunKey, ValueName().c_str(), RRF_RT_REG_SZ, nullptr,
                        nullptr, nullptr) == ERROR_SUCCESS;
}

domain::Status SetAutostart(bool enabled, const std::wstring& commandLine) {
    if (!enabled) {
        const LSTATUS status = RegDeleteKeyValueW(HKEY_CURRENT_USER, kRunKey, ValueName().c_str());
        if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
            return RegistryError(status);
        }
        return domain::Unit{};
    }
    const DWORD bytes = static_cast<DWORD>((commandLine.size() + 1) * sizeof(wchar_t));
    const LSTATUS status = RegSetKeyValueW(HKEY_CURRENT_USER, kRunKey, ValueName().c_str(), REG_SZ,
                                           commandLine.c_str(), bytes);
    if (status != ERROR_SUCCESS) {
        return RegistryError(status);
    }
    return domain::Unit{};
}

}  // namespace et::infra
