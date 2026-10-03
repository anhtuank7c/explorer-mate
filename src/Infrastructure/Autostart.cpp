#include "Infrastructure/Autostart.h"

#include <windows.h>

#include <roapi.h>
#include <windows.applicationmodel.h>
#include <windows.foundation.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>

#include <memory>

#include "Domain/ProductInfo.h"
#include "Infrastructure/ErrorText.h"

// Every binary that links this static library needs the Windows Runtime import library.
#pragma comment(lib, "runtimeobject.lib")

namespace et::infra {

namespace {

using ABI::Windows::ApplicationModel::IStartupTask;
using ABI::Windows::ApplicationModel::IStartupTaskStatics;
using ABI::Windows::ApplicationModel::StartupTask;
using ABI::Windows::ApplicationModel::StartupTaskState;
using ABI::Windows::ApplicationModel::StartupTaskState_Enabled;
using ABI::Windows::ApplicationModel::StartupTaskState_EnabledByPolicy;
using ABI::Windows::Foundation::IAsyncOperation;
using ABI::Windows::Foundation::IAsyncOperationCompletedHandler;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Wrappers::HStringReference;

constexpr const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
// Must match the StartupTask TaskId in packaging\release\AppxManifest.xml.
constexpr wchar_t kStartupTaskId[] = L"ExplorerMateAgent";
constexpr DWORD kAsyncTimeoutMs = 5000;

std::wstring ValueName() {
    return std::wstring(domain::ProductShortName());
}

domain::Error RegistryError(LSTATUS status) {
    return domain::Error(domain::ErrorCode::OperationFailed,
                         L"Cannot change the startup setting: " +
                             DescribeWin32Error(static_cast<unsigned long>(status)));
}

// Waits for a Windows Runtime operation started on this (single-threaded apartment) thread.
// Only COM calls are dispatched while waiting, not window messages, so the caller's window
// procedure is not re-entered.
template <typename T>
HRESULT Await(IAsyncOperation<T>* operation) {
    // Shared with the handler: after a timeout the handler may still run later.
    const std::shared_ptr<void> done(CreateEventW(nullptr, TRUE, FALSE, nullptr), [](HANDLE event) {
        if (event != nullptr) {
            CloseHandle(event);
        }
    });
    if (!done) {
        return E_OUTOFMEMORY;
    }
    const auto handler = Microsoft::WRL::Callback<
        Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                                   IAsyncOperationCompletedHandler<T>, Microsoft::WRL::FtmBase>>(
        [done](IAsyncOperation<T>*, AsyncStatus) {
            SetEvent(done.get());
            return S_OK;
        });
    HRESULT result = handler ? operation->put_Completed(handler.Get()) : E_OUTOFMEMORY;
    if (FAILED(result)) {
        return result;
    }
    HANDLE event = done.get();
    DWORD index = 0;
    result = CoWaitForMultipleHandles(COWAIT_DISPATCH_CALLS, kAsyncTimeoutMs, 1, &event, &index);
    if (FAILED(result)) {
        return result;
    }
    ComPtr<IAsyncInfo> info;
    HRESULT error = E_FAIL;
    if (FAILED(operation->QueryInterface(IID_PPV_ARGS(&info))) || FAILED(info->get_ErrorCode(&error))) {
        return E_FAIL;
    }
    return error;
}

// The startup task declared by the installed package, or null when the program does not run
// from a package that declares one (development builds), in which case the Run key is used.
// A packaged program cannot use the Run key: its registry writes are private to the package.
ComPtr<IStartupTask> PackageStartupTask() {
    ComPtr<IStartupTaskStatics> statics;
    ComPtr<IAsyncOperation<StartupTask*>> operation;
    ComPtr<IStartupTask> task;
    if (FAILED(RoGetActivationFactory(
            HStringReference(RuntimeClass_Windows_ApplicationModel_StartupTask).Get(),
            IID_PPV_ARGS(&statics))) ||
        FAILED(statics->GetAsync(HStringReference(kStartupTaskId).Get(), &operation)) ||
        FAILED(Await(operation.Get())) || FAILED(operation->GetResults(&task))) {
        return nullptr;
    }
    return task;
}

bool IsOn(StartupTaskState state) {
    return state == StartupTaskState_Enabled || state == StartupTaskState_EnabledByPolicy;
}

domain::Status SetStartupTask(IStartupTask* task, bool enabled) {
    if (!enabled) {
        const HRESULT result = task->Disable();
        if (FAILED(result)) {
            return domain::Error(domain::ErrorCode::OperationFailed,
                                 L"Cannot change the startup setting: " + DescribeHresult(result));
        }
        return domain::Unit{};
    }
    ComPtr<IAsyncOperation<StartupTaskState>> operation;
    StartupTaskState state{};
    HRESULT result = task->RequestEnableAsync(&operation);
    if (SUCCEEDED(result)) {
        result = Await(operation.Get());
    }
    if (SUCCEEDED(result)) {
        result = operation->GetResults(&state);
    }
    if (FAILED(result)) {
        return domain::Error(domain::ErrorCode::OperationFailed,
                             L"Cannot change the startup setting: " + DescribeHresult(result));
    }
    if (!IsOn(state)) {
        // Turned off by the user in Windows, or by policy; only Windows can turn it back on.
        return domain::Error(domain::ErrorCode::AccessDenied,
                             L"Windows keeps Explorer Mate turned off at sign-in. Turn it on in "
                             L"Settings > Apps > Startup.");
    }
    return domain::Unit{};
}

}  // namespace

bool IsAutostartEnabled() {
    if (const ComPtr<IStartupTask> task = PackageStartupTask()) {
        StartupTaskState state{};
        return SUCCEEDED(task->get_State(&state)) && IsOn(state);
    }
    return RegGetValueW(HKEY_CURRENT_USER, kRunKey, ValueName().c_str(), RRF_RT_REG_SZ, nullptr,
                        nullptr, nullptr) == ERROR_SUCCESS;
}

domain::Status SetAutostart(bool enabled, const std::wstring& commandLine) {
    if (const ComPtr<IStartupTask> task = PackageStartupTask()) {
        return SetStartupTask(task.Get(), enabled);
    }
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
