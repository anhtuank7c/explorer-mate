#include "ShellExtension/WorkerLauncher.h"

#include "Infrastructure/WorkerProcess.h"

namespace et::ui {

domain::Status LaunchWorker(const app::ActionRequest& request) {
    // Resolved from this module so no search path is involved.
    const std::wstring executable = infra::SiblingPathOfModule(
        reinterpret_cast<const void*>(&LaunchWorker), L"ExMate.exe");
    const auto worker = infra::WorkerProcess::Start(executable, request);
    if (!worker.ok()) {
        return worker.error();
    }
    return domain::Unit{};
}

}  // namespace et::ui
