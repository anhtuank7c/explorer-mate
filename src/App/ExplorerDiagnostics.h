#pragma once

namespace et::ui {

// Prints every open Explorer tab, which one is active in its frame, where the keyboard focus
// is, and each tab's selection. Used to verify tab detection before hotkeys rely on it.
// `delaySeconds` gives time to switch to the window under test before the snapshot.
void PrintExplorerDiagnostics(unsigned delaySeconds);

// Records, for `seconds`, one line every time the foreground window, the keyboard focus, the
// active tab or its selection changes. Lines go to stdout and to
// %LOCALAPPDATA%\ExMate\logs\tab-watch.log so a person can click through a scenario
// and the result can be read afterwards.
void WatchExplorer(unsigned seconds);

}  // namespace et::ui
