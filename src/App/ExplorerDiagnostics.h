#pragma once

namespace et::ui {

// Prints every open Explorer tab, which one is active in its frame, where the keyboard focus
// is, and each tab's selection. Used to verify tab detection before hotkeys rely on it.
// `delaySeconds` gives time to switch to the window under test before the snapshot.
void PrintExplorerDiagnostics(unsigned delaySeconds);

// Records, for `seconds`, one line every time the foreground window, the keyboard focus, the
// active tab or its selection changes, so a person can click through a scenario. Lines go
// to stdout only: they contain file names and are not kept in a log file.
void WatchExplorer(unsigned seconds);

}  // namespace et::ui
