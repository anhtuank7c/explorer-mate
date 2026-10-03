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

constexpr size_t kMaxMessageLength = 2000;
constexpr LONGLONG kMaxLogBytes = 1024 * 1024;

// One log entry is one line: text taken from files or other processes must not be able to
// forge extra entries or bloat the log.
std::wstring SingleLine(std::wstring_view message) {
    std::wstring line(message.substr(0, kMaxMessageLength));
    for (wchar_t& character : line) {
        if (character == L'\r' || character == L'\n') {
            character = L' ';
        }
    }
    return line;
}

// Keeps the log bounded: once it passes the limit it becomes "<name>.old" (replacing the
// previous one) and a fresh file is started.
void RotateIfLarge(const std::wstring& filePath) {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &data)) {
        return;
    }
    const LONGLONG size = (static_cast<LONGLONG>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    if (size > kMaxLogBytes) {
        MoveFileExW(filePath.c_str(), (filePath + L".old").c_str(), MOVEFILE_REPLACE_EXISTING);
    }
}

}  // namespace

FileLogger::FileLogger(std::wstring filePath) : filePath_(std::move(filePath)) {}

void FileLogger::Write(app::LogLevel level, std::wstring_view message) {
    std::wstring line = UtcTimestamp();
    line.append(L" [").append(LevelName(level)).append(L"] ").append(SingleLine(message));
    line.append(L"\r\n");
    const std::string utf8 = ToUtf8(line);

    EnsureParentDirectory(filePath_);
    RotateIfLarge(filePath_);
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
