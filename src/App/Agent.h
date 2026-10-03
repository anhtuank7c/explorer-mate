#pragma once

namespace et::ui {

// Runs the background agent until the user exits it from the tray icon: watches the keyboard
// for the configured shortcuts and, when one is pressed in a File Explorer file list, starts
// a worker for that tab's selection. Returns the process exit code. A second agent in the
// same session exits immediately. The calling thread must be in a COM single-threaded
// apartment; it becomes the agent's message loop.
int RunAgent();

// Asks the agent running in this session to exit, as if "Exit" was chosen from its tray
// menu, and waits briefly for it. Returns false when no agent is running.
bool StopRunningAgent();

}  // namespace et::ui
