#pragma once

#include <string>
#include <vector>

namespace et::ui {

// Arguments of the current process, excluding the executable path.
std::vector<std::wstring> ReadProcessArguments();

// Writes a line of text: as UTF-8 to stdout when it is redirected, otherwise to the console
// of the terminal the program was started from, if any. The program is a GUI-subsystem
// executable, so it has no console of its own.
void WriteLineToStdout(const std::wstring& text);

// An item given on the command line as a full path: relative paths are resolved against the
// current folder and a trailing backslash (left by tab completion on folders) is dropped.
std::wstring AbsoluteItemPath(const std::wstring& argument);

}  // namespace et::ui
