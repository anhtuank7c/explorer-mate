#pragma once

#include <string_view>

namespace et::app {

enum class LogLevel {
    Info,
    Warning,
    Error,
};

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void Write(LogLevel level, std::wstring_view message) = 0;
};

}  // namespace et::app
