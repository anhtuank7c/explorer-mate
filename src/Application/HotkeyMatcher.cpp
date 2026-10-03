#include "Application/HotkeyMatcher.h"

#include <utility>

namespace et::app {

namespace {

// Windows virtual-key codes of the modifier keys.
constexpr unsigned kLeftShift = 0xA0;
constexpr unsigned kRightShift = 0xA1;
constexpr unsigned kLeftControl = 0xA2;
constexpr unsigned kRightControl = 0xA3;
constexpr unsigned kLeftAlt = 0xA4;
constexpr unsigned kRightAlt = 0xA5;  // AltGr on layouts that have one.
constexpr unsigned kLeftWin = 0x5B;
constexpr unsigned kRightWin = 0x5C;
// Generic codes some drivers report instead of the left/right ones.
constexpr unsigned kShift = 0x10;
constexpr unsigned kControl = 0x11;
constexpr unsigned kAlt = 0x12;

bool IsModifier(unsigned key) {
    switch (key) {
        case kLeftShift:
        case kRightShift:
        case kLeftControl:
        case kRightControl:
        case kLeftAlt:
        case kRightAlt:
        case kLeftWin:
        case kRightWin:
        case kShift:
        case kControl:
        case kAlt:
            return true;
        default:
            return false;
    }
}

}  // namespace

HotkeyMatcher::HotkeyMatcher(std::vector<HotkeyBinding> bindings)
    : bindings_(std::move(bindings)) {}

void HotkeyMatcher::Reset() {
    heldModifiers_.clear();
    swallowedKey_.reset();
}

std::optional<ActionKind> HotkeyMatcher::MatchChord(unsigned key) const {
    const auto held = [&](unsigned modifier) { return heldModifiers_.count(modifier) != 0; };
    if (held(kRightAlt)) {
        return std::nullopt;
    }
    domain::KeyChord pressed;
    pressed.ctrl = held(kLeftControl) || held(kRightControl) || held(kControl);
    pressed.alt = held(kLeftAlt) || held(kAlt);
    pressed.shift = held(kLeftShift) || held(kRightShift) || held(kShift);
    pressed.win = held(kLeftWin) || held(kRightWin);
    pressed.key = key;

    for (const HotkeyBinding& binding : bindings_) {
        if (binding.chord == pressed) {
            return binding.action;
        }
    }
    return std::nullopt;
}

KeyDecision HotkeyMatcher::OnKey(const KeyEvent& event,
                                 const std::function<bool()>& contextAccepts) {
    if (event.isInjected) {
        return {};
    }
    if (IsModifier(event.virtualKey)) {
        if (event.isDown) {
            heldModifiers_.insert(event.virtualKey);
        } else {
            heldModifiers_.erase(event.virtualKey);
        }
        return {};
    }

    if (!event.isDown) {
        if (swallowedKey_ == event.virtualKey) {
            swallowedKey_.reset();
            return {true, std::nullopt};
        }
        return {};
    }

    if (swallowedKey_ == event.virtualKey) {
        return {true, std::nullopt};  // Auto-repeat while the shortcut is held.
    }
    const auto action = MatchChord(event.virtualKey);
    if (!action || !contextAccepts()) {
        return {};
    }
    swallowedKey_ = event.virtualKey;
    return {true, action};
}

}  // namespace et::app
