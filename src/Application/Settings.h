#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Application/ActionKind.h"
#include "Domain/Error.h"
#include "Domain/KeyChord.h"
#include "Domain/Result.h"

namespace et::app {

struct HotkeyBinding {
    ActionKind action = ActionKind::DuplicateInPlace;
    domain::KeyChord chord;
};

// User preferences of the hotkey agent.
struct Settings {
    bool hotkeysEnabled = true;
    // At most one binding per action. An action without a binding has no shortcut.
    std::vector<HotkeyBinding> hotkeys;

    // Ctrl+Alt+N (group), Ctrl+Alt+R (rename), Ctrl+Alt+D (duplicate). Ctrl+D is avoided on
    // purpose: Explorer uses it for Delete.
    static Settings Defaults();

    std::optional<domain::KeyChord> ChordFor(ActionKind action) const;
};

// The reason the settings cannot be used: an action bound twice, an invalid chord, or the
// same chord on two actions.
std::optional<domain::Error> ValidateSettings(const Settings& settings);

// Line-based text:
//   ExMate-Settings 1
//   hotkeys=on|off
//   <action wire name>=<chord>|none
std::wstring SerializeSettings(const Settings& settings);
domain::Result<Settings> ParseSettings(std::wstring_view text);

}  // namespace et::app
