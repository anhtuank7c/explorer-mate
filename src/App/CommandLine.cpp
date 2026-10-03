#include "App/CommandLine.h"

#include <windows.h>

#include <shellapi.h>

#include "Infrastructure/Utf8.h"

namespace et::ui {

std::vector<std::wstring> ReadProcessArguments() {
    int count = 0;
    wchar_t** raw = CommandLineToArgvW(GetCommandLineW(), &count);
    if (raw == nullptr) {
        return {};
    }
    std::vector<std::wstring> arguments;
    for (int index = 1; index < count; ++index) {
        arguments.emplace_back(raw[index]);
    }
    LocalFree(raw);
    return arguments;
}

namespace {

// The console of the terminal that started the program, or an invalid handle when there is
// none (started from Explorer, the tray or the shell extension).
HANDLE ParentConsole() {
    static const HANDLE console = []() -> HANDLE {
        // Fails harmlessly when a console is already attached.
        AttachConsole(ATTACH_PARENT_PROCESS);
        return CreateFileW(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    }();
    return console;
}

}  // namespace

void WriteLineToStdout(const std::wstring& text) {
    DWORD written = 0;
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    const DWORD type = (output == nullptr || output == INVALID_HANDLE_VALUE)
                           ? FILE_TYPE_UNKNOWN
                           : GetFileType(output);
    if (type == FILE_TYPE_DISK || type == FILE_TYPE_PIPE) {
        const std::string utf8 = infra::ToUtf8(text) + "\n";
        WriteFile(output, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
        return;
    }
    const HANDLE console = ParentConsole();
    if (console != INVALID_HANDLE_VALUE) {
        const std::wstring line = text + L"\r\n";
        WriteConsoleW(console, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
    }
}

std::wstring AbsoluteItemPath(const std::wstring& argument) {
    const DWORD needed = GetFullPathNameW(argument.c_str(), 0, nullptr, nullptr);
    if (needed == 0) {
        return argument;
    }
    std::wstring path(needed, L'\0');
    const DWORD length = GetFullPathNameW(argument.c_str(), needed, path.data(), nullptr);
    if (length == 0 || length >= needed) {
        return argument;
    }
    path.resize(length);
    // "C:\" must keep its backslash; "C:\Work\" must lose it.
    while (path.size() > 3 && path.back() == L'\\') {
        path.pop_back();
    }
    return path;
}

}  // namespace et::ui
