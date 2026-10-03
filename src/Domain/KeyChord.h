#pragma once

#include <string>
#include <string_view>

#include "Domain/Result.h"

namespace et::domain {

// A keyboard shortcut: modifiers plus one key, e.g. Ctrl+Alt+N. `key` is a Windows
// virtual-key code; only letters, digits and F1..F24 are accepted so the text form is
// unambiguous on every keyboard layout.
struct KeyChord {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool win = false;
    unsigned key = 0;

    friend bool operator==(const KeyChord&, const KeyChord&) = default;
};

// "Ctrl+Alt+N". Modifiers are always written in the order Ctrl, Alt, Shift, Win.
std::wstring FormatKeyChord(const KeyChord& chord);

// Accepts the format above, case-insensitively and with modifiers in any order. A chord
// needs Ctrl, Alt or Win: Shift+letter and bare keys would hijack normal typing.
Result<KeyChord> ParseKeyChord(std::wstring_view text);

// Same rules for a chord built from raw parts (e.g. from a hotkey control).
Result<KeyChord> ValidateKeyChord(const KeyChord& chord);

}  // namespace et::domain
