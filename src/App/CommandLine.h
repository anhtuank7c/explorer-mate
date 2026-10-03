#pragma once

#include <string>
#include <vector>

namespace et::ui {

// Arguments of the current process, excluding the executable path.
std::vector<std::wstring> ReadProcessArguments();

// Writes UTF-8 text to stdout when the process has one (redirected or inherited console).
void WriteLineToStdout(const std::wstring& text);

}  // namespace et::ui
