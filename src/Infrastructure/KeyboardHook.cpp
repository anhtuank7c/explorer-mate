#include "Infrastructure/KeyboardHook.h"

#include <windows.h>

#include <utility>

namespace et::infra {

namespace {

// The hook procedure is a plain function, so the single live hook is reachable through
// these. Only touched on the installing thread.
HHOOK g_hook = nullptr;
KeyboardHook::Handler g_handler;

LRESULT CALLBACK HookProcedure(int code, WPARAM message, LPARAM data) {
    if (code == HC_ACTION && g_handler) {
        const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
        app::KeyEvent event;
        event.virtualKey = key->vkCode;
        event.isDown = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
        event.isInjected = (key->flags & LLKHF_INJECTED) != 0;
        bool swallow = false;
        try {
            swallow = g_handler(event);
        } catch (...) {
            swallow = false;  // Never let a failure eat the user's keystrokes.
        }
        if (swallow) {
            return 1;
        }
    }
    return CallNextHookEx(nullptr, code, message, data);
}

}  // namespace

KeyboardHook::KeyboardHook(Handler handler) {
    if (g_hook != nullptr) {
        return;
    }
    g_handler = std::move(handler);
    g_hook = SetWindowsHookExW(WH_KEYBOARD_LL, HookProcedure, GetModuleHandleW(nullptr), 0);
    if (g_hook == nullptr) {
        g_handler = nullptr;
    }
}

KeyboardHook::~KeyboardHook() {
    if (g_hook != nullptr) {
        UnhookWindowsHookEx(g_hook);
        g_hook = nullptr;
        g_handler = nullptr;
    }
}

bool KeyboardHook::installed() const {
    return g_hook != nullptr;
}

bool PhysicalModifiersMatch(const domain::KeyChord& chord) {
    const auto down = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; };
    return down(VK_CONTROL) == chord.ctrl && down(VK_MENU) == chord.alt &&
           down(VK_SHIFT) == chord.shift && (down(VK_LWIN) || down(VK_RWIN)) == chord.win;
}

}  // namespace et::infra
