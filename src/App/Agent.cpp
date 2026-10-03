#include "App/Agent.h"

#include <windows.h>

#include <shellapi.h>
#include <wtsapi32.h>

#include <optional>
#include <string>
#include <utility>

#include "App/AboutDialog.h"
#include "App/AppIcon.h"
#include "App/SettingsDialog.h"
#include "Application/HotkeyMatcher.h"
#include "Application/Settings.h"
#include "Domain/ProductInfo.h"
#include "Infrastructure/AppDataPaths.h"
#include "Infrastructure/Autostart.h"
#include "Infrastructure/ExplorerSelectionSource.h"
#include "Infrastructure/FileLogger.h"
#include "Infrastructure/KeyboardHook.h"
#include "Infrastructure/SettingsFile.h"
#include "Infrastructure/WorkerProcess.h"

namespace et::ui {

namespace {

constexpr const wchar_t* kWindowClass = L"ExplorerMateAgentWindow";
constexpr const wchar_t* kInstanceMutex = L"Local\\ExplorerMate.Agent";
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kHotkeyMessage = WM_APP + 2;
constexpr UINT kTrayIconId = 1;

enum MenuCommand : UINT {
    kToggleHotkeys = 1,
    kOpenSettings,
    kToggleAutostart,
    kAbout,
    kExit,
};

std::wstring DataPath(const wchar_t* relative) {
    const auto directory = infra::ProductDataDirectory();
    return (directory.ok() ? directory.value() : std::wstring(L".")) + relative;
}

class Agent {
public:
    Agent()
        : log_(DataPath(L"\\logs\\agent.log")),
          settingsFile_(DataPath(L"\\settings.txt")),
          executable_(infra::SiblingPathOfModule(reinterpret_cast<const void*>(&RunAgent),
                                                 L"ExplorerMate.exe")) {}

    int Run() {
        LoadSettings();
        if (!CreateAgentWindow()) {
            log_.Write(app::LogLevel::Error, L"Cannot create the agent window.");
            return 1;
        }
        AddTrayIcon();
        WTSRegisterSessionNotification(window_, NOTIFY_FOR_THIS_SESSION);
        hook_.emplace([this](const app::KeyEvent& event) { return OnKey(event); });
        if (!hook_->installed()) {
            log_.Write(app::LogLevel::Error, L"Cannot install the keyboard hook.");
        }
        log_.Write(app::LogLevel::Info, L"Agent started.");

        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        hook_.reset();
        log_.Write(app::LogLevel::Info, L"Agent stopped.");
        return 0;
    }

private:
    // --- Settings ------------------------------------------------------------------------
    void LoadSettings() {
        auto loaded = settingsFile_.Load();
        if (loaded.ok()) {
            ApplySettings(std::move(loaded).value());
            return;
        }
        // Keep the unreadable file untouched; run on defaults for this session.
        log_.Write(app::LogLevel::Error, L"Settings ignored: " + loaded.error().message);
        ApplySettings(app::Settings::Defaults());
    }

    void ApplySettings(app::Settings settings) {
        settings_ = std::move(settings);
        matcher_.emplace(settings_.hotkeys);
    }

    void SaveSettings() {
        if (const auto saved = settingsFile_.Save(settings_); !saved.ok()) {
            log_.Write(app::LogLevel::Error, saved.error().message);
        }
    }

    // --- Keyboard ------------------------------------------------------------------------
    // Runs inside the low-level hook: must stay fast and must not touch COM.
    bool OnKey(const app::KeyEvent& event) {
        bool staleModifiers = false;
        const app::KeyDecision decision =
            matcher_->OnKey(event, [&](const domain::KeyChord& chord) {
                // The matcher's modifier state comes from events and misses key-ups that
                // happened behind a UAC prompt or an elevated window. Never act on it alone.
                if (!infra::PhysicalModifiersMatch(chord)) {
                    staleModifiers = true;
                    return false;
                }
                return CanAct();
            });
        if (staleModifiers) {
            matcher_->Reset();
        }
        if (decision.triggered) {
            // The action stays in this process. The posted message is only a wake-up, so
            // another program posting it cannot make the agent act: there is nothing pending.
            pendingAction_ = decision.triggered;
            PostMessageW(window_, kHotkeyMessage, 0, 0);
        }
        return decision.swallow;
    }

    bool ShortcutsAllowed() const {
        return settings_.hotkeysEnabled && !editingSettings_ && !handlingHotkey_ &&
               !worker_.IsRunning();
    }

    // One command at a time: while a worker (and its dialog) is alive, shortcuts are not
    // intercepted at all and reach Explorer as ordinary keys.
    bool CanAct() const {
        return ShortcutsAllowed() && selection_.FocusIsInFileList();
    }

    void OnHotkeyMessage() {
        const std::optional<app::ActionKind> action = std::exchange(pendingAction_, std::nullopt);
        if (!action || !ShortcutsAllowed()) {
            return;
        }
        // Reading the selection makes cross-process COM calls, which pump messages: without
        // this flag a second wake-up could re-enter here before the worker is recorded.
        handlingHotkey_ = true;
        StartWorkerForFocusedSelection(*action);
        handlingHotkey_ = false;
    }

    void StartWorkerForFocusedSelection(app::ActionKind action) {
        auto paths = selection_.CaptureFocusedSelection();
        if (!paths.ok()) {
            // Refusing is the safe outcome; an empty selection is not worth a log line.
            if (paths.error().code != domain::ErrorCode::EmptySelection) {
                log_.Write(app::LogLevel::Warning, L"Shortcut ignored: " + paths.error().message);
            }
            return;
        }
        auto worker = infra::WorkerProcess::Start(executable_, {action, std::move(paths).value()});
        if (!worker.ok()) {
            log_.Write(app::LogLevel::Error, worker.error().message);
            return;
        }
        worker_ = std::move(worker).value();
    }

    // --- Tray ----------------------------------------------------------------------------
    void AddTrayIcon() {
        NOTIFYICONDATAW icon{};
        icon.cbSize = sizeof(icon);
        icon.hWnd = window_;
        icon.uID = kTrayIconId;
        icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        icon.uCallbackMessage = kTrayMessage;
        icon.hIcon = SmallAppIcon();
        wcscpy_s(icon.szTip, std::wstring(domain::ProductName()).c_str());
        Shell_NotifyIconW(NIM_ADD, &icon);
    }

    void RemoveTrayIcon() {
        NOTIFYICONDATAW icon{};
        icon.cbSize = sizeof(icon);
        icon.hWnd = window_;
        icon.uID = kTrayIconId;
        Shell_NotifyIconW(NIM_DELETE, &icon);
    }

    void ShowTrayMenu() {
        const HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING | (settings_.hotkeysEnabled ? MF_CHECKED : 0), kToggleHotkeys,
                    L"Keyboard shortcuts enabled");
        AppendMenuW(menu, MF_STRING, kOpenSettings, L"Settings...");
        AppendMenuW(menu, MF_STRING | (infra::IsAutostartEnabled() ? MF_CHECKED : 0),
                    kToggleAutostart, L"Start with Windows");
        AppendMenuW(menu, MF_STRING, kAbout, L"About...");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kExit, L"Exit");

        POINT cursor{};
        GetCursorPos(&cursor);
        // Required so the menu closes when the user clicks elsewhere.
        SetForegroundWindow(window_);
        const UINT command = static_cast<UINT>(TrackPopupMenu(
            menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, window_, nullptr));
        DestroyMenu(menu);
        OnMenuCommand(command);
    }

    void OnMenuCommand(UINT command) {
        switch (command) {
            case kToggleHotkeys:
                settings_.hotkeysEnabled = !settings_.hotkeysEnabled;
                SaveSettings();
                break;
            case kOpenSettings:
                OpenSettings();
                break;
            case kToggleAutostart:
                ToggleAutostart();
                break;
            case kAbout:
                OpenAbout();
                break;
            case kExit:
                DestroyWindow(window_);
                break;
            default:
                break;
        }
    }

    void OpenSettings() {
        if (editingSettings_) {
            return;
        }
        editingSettings_ = true;
        const auto edited = EditSettings(settings_);
        editingSettings_ = false;
        if (edited) {
            ApplySettings(*edited);
            SaveSettings();
        }
    }

    void OpenAbout() {
        if (showingAbout_) {
            return;  // The tray menu stays reachable while the window is open.
        }
        showingAbout_ = true;
        ShowAbout();
        showingAbout_ = false;
    }

    void ToggleAutostart() {
        const auto changed = infra::SetAutostart(!infra::IsAutostartEnabled(),
                                                 L"\"" + executable_ + L"\" --agent");
        if (!changed.ok()) {
            log_.Write(app::LogLevel::Error, changed.error().message);
        }
    }

    // --- Window --------------------------------------------------------------------------
    bool CreateAgentWindow() {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = WindowProcedure;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = kWindowClass;
        RegisterClassExW(&windowClass);
        taskbarCreated_ = RegisterWindowMessageW(L"TaskbarCreated");
        // A hidden top-level window rather than a message-only one: it must receive the
        // TaskbarCreated broadcast to restore the tray icon after Explorer restarts.
        window_ = CreateWindowExW(0, kWindowClass, std::wstring(domain::ProductName()).c_str(),
                                  WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr,
                                  GetModuleHandleW(nullptr), this);
        return window_ != nullptr;
    }

    static LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam,
                                            LPARAM lParam) {
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(window, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }
        auto* agent = reinterpret_cast<Agent*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (agent == nullptr) {
            return DefWindowProcW(window, message, wParam, lParam);
        }
        return agent->HandleMessage(window, message, wParam, lParam);
    }

    LRESULT HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == taskbarCreated_ && taskbarCreated_ != 0) {
            AddTrayIcon();
            return 0;
        }
        switch (message) {
            case kHotkeyMessage:
                OnHotkeyMessage();
                return 0;
            case kTrayMessage:
                if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_LBUTTONUP) {
                    ShowTrayMenu();
                }
                return 0;
            case WM_WTSSESSION_CHANGE:
                // Key-ups are missed while the session is locked or switched away.
                matcher_->Reset();
                return 0;
            case WM_DESTROY:
                WTSUnRegisterSessionNotification(window);
                RemoveTrayIcon();
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProcW(window, message, wParam, lParam);
        }
    }

    infra::FileLogger log_;
    infra::SettingsFile settingsFile_;
    std::wstring executable_;
    infra::ExplorerSelectionSource selection_;
    app::Settings settings_;
    std::optional<app::HotkeyMatcher> matcher_;
    std::optional<infra::KeyboardHook> hook_;
    infra::WorkerProcess worker_;
    HWND window_ = nullptr;
    UINT taskbarCreated_ = 0;
    std::optional<app::ActionKind> pendingAction_;  // Set by the hook, consumed by the wake-up.
    bool handlingHotkey_ = false;
    bool editingSettings_ = false;
    bool showingAbout_ = false;
};

}  // namespace

bool StopRunningAgent() {
    const HWND window = FindWindowW(kWindowClass, nullptr);
    if (window == nullptr) {
        return false;
    }
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    const HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, processId);
    PostMessageW(window, WM_CLOSE, 0, 0);
    if (process != nullptr) {
        // Callers replace the executable next; give the agent a moment to let go of it.
        WaitForSingleObject(process, 5000);
        CloseHandle(process);
    }
    return true;
}

int RunAgent() {
    const HANDLE instance = CreateMutexW(nullptr, TRUE, kInstanceMutex);
    if (instance == nullptr || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (instance != nullptr) {
            CloseHandle(instance);
        }
        // Normally another agent already serves this session. If its window is missing,
        // something else holds the name and shortcuts will not work: leave a trace.
        if (FindWindowW(kWindowClass, nullptr) == nullptr) {
            infra::FileLogger(DataPath(L"\\logs\\agent.log"))
                .Write(app::LogLevel::Error,
                       L"Agent not started: the single-instance name is taken but no agent "
                       L"window exists.");
        }
        return 0;
    }
    const int exitCode = Agent().Run();
    ReleaseMutex(instance);
    CloseHandle(instance);
    return exitCode;
}

}  // namespace et::ui
