#pragma once

#include <functional>
#include <optional>
#include <set>
#include <vector>

#include "Application/ActionKind.h"
#include "Application/Settings.h"

namespace et::app {

struct KeyEvent {
    unsigned virtualKey = 0;
    bool isDown = false;
    bool isInjected = false;  // Synthesised by software rather than typed.
};

struct KeyDecision {
    bool swallow = false;                 // True: the key must not reach the application.
    std::optional<ActionKind> triggered;  // Set once, on the key-down that completes a chord.
};

// Decides, key by key, whether the user just pressed one of the configured shortcuts.
// Designed for a low-level keyboard hook: no I/O, constant work per key, and it fails
// closed - anything unusual lets the key through untouched.
//
// Rules:
//  - Modifier state is tracked from the events themselves and modifiers are never swallowed,
//    so a modifier can never get "stuck" in the application.
//  - A chord matches only with exactly its modifiers held.
//  - Right Alt is treated as AltGr and never counts as Alt: on many layouts AltGr+letter
//    types a character.
//  - Auto-repeat of the shortcut key is swallowed without triggering again, and the matching
//    key-up is swallowed too so the application never sees half a keystroke.
//  - Injected events are ignored.
class HotkeyMatcher {
public:
    explicit HotkeyMatcher(std::vector<HotkeyBinding> bindings);

    // `contextAccepts` is asked only when a chord matches: is the keyboard focus somewhere
    // the shortcut applies? It runs inside the hook and must be fast.
    KeyDecision OnKey(const KeyEvent& event, const std::function<bool()>& contextAccepts);

    // Forgets held keys, e.g. after the session was locked and key-ups were missed.
    void Reset();

private:
    std::optional<ActionKind> MatchChord(unsigned key) const;

    std::vector<HotkeyBinding> bindings_;
    std::set<unsigned> heldModifiers_;
    std::optional<unsigned> swallowedKey_;
};

}  // namespace et::app
