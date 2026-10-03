#pragma once

#include <windows.h>

#include <string>

#include "Application/ActionRequest.h"
#include "Domain/Result.h"

namespace et::infra {

// A started ExMate.exe worker carrying out one request. Move-only; owns the process
// handle. Destroying it does not stop the worker.
class WorkerProcess {
public:
    // Writes the request to the per-user request folder and starts `executable` (absolute
    // path) with "--request <file>". Returns as soon as the process exists.
    static domain::Result<WorkerProcess> Start(const std::wstring& executable,
                                               const app::ActionRequest& request);

    WorkerProcess() = default;
    WorkerProcess(WorkerProcess&& other) noexcept;
    WorkerProcess& operator=(WorkerProcess&& other) noexcept;
    ~WorkerProcess();

    bool IsRunning() const;

private:
    explicit WorkerProcess(HANDLE process) : process_(process) {}
    void Close();

    HANDLE process_ = nullptr;
};

// Absolute path of a file located next to the module containing `addressInModule`.
std::wstring SiblingPathOfModule(const void* addressInModule, const wchar_t* fileName);

}  // namespace et::infra
