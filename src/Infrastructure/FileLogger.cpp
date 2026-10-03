#include "Infrastructure/FileLogger.h"

#include <windows.h>

#include <cwchar>
#include <filesystem>
#include <system_error>
#include <utility>

#include "Infrastructure/Utf8.h"

namespace et::infra {

namespace {

std::wstring_view LevelName(app::LogLevel level) {
    switch (level) {
        case app::LogLevel::Info:
            return L"INFO";
        case app::LogLevel::Warning:
            return L"WARN";
        case app::LogLevel::Error:
            return L"ERROR";
    }
    return L"INFO";
}

std::wstring UtcTimestamp() {
    SYSTEMTIME now{};
    GetSystemTime(&now);
    wchar_t buffer[32]{};
    swprintf_s(buffer, L"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ", now.wYear, now.wMonth, now.wDay,
               now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
    return buffer;
}

void EnsureParentDirectory(const std::wstring& filePath) {
    std::error_code ignored;
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path(), ignored);
}

}  // namespace

FileLogger::FileLogger(std::wstring filePath) : filePath_(std::move(filePath)) {}

void FileLogger::Write(app::LogLevel level, std::wstring_view message) {
    std::wstring line = UtcTimestamp();
    line.append(L" [").append(LevelName(level)).append(L"] ").append(message).append(L"\r\n");
    const std::string utf8 = ToUtf8(line);

    EnsureParentDirectory(filePath_);
    const HANDLE file = CreateFileW(filePath_.c_str(), FILE_APPEND_DATA,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD written = 0;
    WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    CloseHandle(file);
}

}  // namespace et::infra
