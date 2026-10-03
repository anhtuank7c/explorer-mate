#include "Infrastructure/WorkerProcess.h"

#include <utility>

#include "Infrastructure/AppDataPaths.h"
#include "Infrastructure/ErrorText.h"
#include "Infrastructure/RequestFileStore.h"

namespace et::infra {

namespace {

domain::Result<HANDLE> CreateWorker(const std::wstring& executable, const std::wstring& requestFile) {
    // Request files are named by GUID inside a known folder, so plain quoting is sufficient.
    std::wstring commandLine = L"\"" + executable + L"\" --request \"" + requestFile + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        nullptr, &startup, &process)) {
        return domain::Error(domain::ErrorCode::OperationFailed,
                             L"Cannot start " + executable + L": " +
                                 DescribeWin32Error(GetLastError()));
    }
    // Pass on whatever right this process has to take the foreground, so the worker's dialog
    // opens in front. Harmless when the right is absent.
    AllowSetForegroundWindow(process.dwProcessId);
    CloseHandle(process.hThread);
    return process.hProcess;
}

}  // namespace

domain::Result<WorkerProcess> WorkerProcess::Start(const std::wstring& executable,
                                                   const app::ActionRequest& request) {
    const auto dataDirectory = ProductDataDirectory();
    if (!dataDirectory.ok()) {
        return dataDirectory.error();
    }
    if (executable.empty()) {
        return domain::Error(domain::ErrorCode::OperationFailed, L"Cannot locate the worker.");
    }
    const RequestFileStore store(dataDirectory.value() + L"\\requests");
    const auto requestFile = store.Put(request);
    if (!requestFile.ok()) {
        return requestFile.error();
    }
    const auto process = CreateWorker(executable, requestFile.value());
    if (!process.ok()) {
        DeleteFileW(requestFile.value().c_str());
        return process.error();
    }
    return WorkerProcess(process.value());
}

WorkerProcess::WorkerProcess(WorkerProcess&& other) noexcept
    : process_(std::exchange(other.process_, nullptr)) {}

WorkerProcess& WorkerProcess::operator=(WorkerProcess&& other) noexcept {
    if (this != &other) {
        Close();
        process_ = std::exchange(other.process_, nullptr);
    }
    return *this;
}

WorkerProcess::~WorkerProcess() {
    Close();
}

void WorkerProcess::Close() {
    if (process_ != nullptr) {
        CloseHandle(process_);
        process_ = nullptr;
    }
}

bool WorkerProcess::IsRunning() const {
    return process_ != nullptr && WaitForSingleObject(process_, 0) == WAIT_TIMEOUT;
}

std::wstring SiblingPathOfModule(const void* addressInModule, const wchar_t* fileName) {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            static_cast<LPCWSTR>(addressInModule), &module)) {
        return {};
    }
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0) {
            return {};
        }
        if (length < path.size()) {
            path.resize(length);
            break;
        }
        path.resize(path.size() * 2);
    }
    return path.substr(0, path.find_last_of(L'\\') + 1) + fileName;
}

}  // namespace et::infra
