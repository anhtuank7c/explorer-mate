#pragma once

#include <string>

#include "Application/Ports/ILogger.h"

namespace et::infra {

// Appends UTF-8 lines to one file. The file is opened per write so several processes
// (shell host, worker) can share it. Logging failures are swallowed by design. Each message
// becomes exactly one line, and the file is rotated to "<name>.old" at about 1 MB.
class FileLogger final : public app::ILogger {
public:
    explicit FileLogger(std::wstring filePath);

    void Write(app::LogLevel level, std::wstring_view message) override;

private:
    std::wstring filePath_;
};

}  // namespace et::infra
