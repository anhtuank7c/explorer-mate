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

void WriteLineToStdout(const std::wstring& text) {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == nullptr || output == INVALID_HANDLE_VALUE) {
        return;
    }
    const std::string utf8 = infra::ToUtf8(text) + "\n";
    DWORD written = 0;
    WriteFile(output, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
}

}  // namespace et::ui
