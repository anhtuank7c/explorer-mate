#include "ShellExtension/ShellLog.h"

#include <memory>
#include <string>

#include "Infrastructure/AppDataPaths.h"
#include "Infrastructure/FileLogger.h"

namespace et::ui {

namespace {

class NullLogger final : public app::ILogger {
public:
    void Write(app::LogLevel, std::wstring_view) override {}
};

std::unique_ptr<app::ILogger> CreateShellLog() {
    const auto dataDirectory = infra::ProductDataDirectory();
    if (!dataDirectory.ok()) {
        return std::make_unique<NullLogger>();
    }
    return std::make_unique<infra::FileLogger>(dataDirectory.value() + L"\\logs\\shell.log");
}

}  // namespace

app::ILogger& ShellLog() {
    static const std::unique_ptr<app::ILogger> logger = CreateShellLog();
    return *logger;
}

}  // namespace et::ui
