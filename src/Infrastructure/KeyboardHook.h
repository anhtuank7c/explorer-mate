#pragma once

#include <functional>

#include "Application/HotkeyMatcher.h"

namespace et::infra {

// System-wide low-level keyboard hook (WH_KEYBOARD_LL). The handler runs on the thread that
// installed the hook, which must pump messages, and must return quickly: Windows silently
// removes hooks that stall the input queue. Return true from the handler to swallow the key.
// Only one instance may exist at a time.
class KeyboardHook {
public:
    using Handler = std::function<bool(const app::KeyEvent&)>;

    explicit KeyboardHook(Handler handler);
    ~KeyboardHook();

    KeyboardHook(const KeyboardHook&) = delete;
    KeyboardHook& operator=(const KeyboardHook&) = delete;

    bool installed() const;
};

}  // namespace et::infra
