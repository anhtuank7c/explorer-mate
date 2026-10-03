#include "Application/Settings.h"

#include <array>
#include <utility>

namespace et::app {

namespace {

constexpr std::wstring_view kHeader = L"ExMate-Settings 1";
constexpr std::wstring_view kHotkeysKey = L"hotkeys";
constexpr std::wstring_view kNoChord = L"none";
constexpr std::array<ActionKind, 3> kAllActions{ActionKind::GroupIntoNewFolder,
                                                ActionKind::BulkRename,
                                                ActionKind::DuplicateInPlace};

domain::Error SettingsError(std::wstring message) {
    return domain::Error(domain::ErrorCode::InvalidArgument, std::move(message));
}

domain::KeyChord CtrlAlt(wchar_t key) {
    domain::KeyChord chord;
    chord.ctrl = true;
    chord.alt = true;
    chord.key = static_cast<unsigned>(key);
    return chord;
}

std::wstring_view TakeLine(std::wstring_view& text) {
    const size_t end = text.find(L'\n');
    std::wstring_view line = text.substr(0, end);
    text.remove_prefix(end == std::wstring_view::npos ? text.size() : end + 1);
    if (!line.empty() && line.back() == L'\r') {
        line.remove_suffix(1);
    }
    return line;
}

}  // namespace

Settings Settings::Defaults() {
    Settings settings;
    settings.hotkeys = {{ActionKind::GroupIntoNewFolder, CtrlAlt(L'N')},
                        {ActionKind::BulkRename, CtrlAlt(L'R')},
                        {ActionKind::DuplicateInPlace, CtrlAlt(L'D')}};
    return settings;
}

std::optional<domain::KeyChord> Settings::ChordFor(ActionKind action) const {
    for (const HotkeyBinding& binding : hotkeys) {
        if (binding.action == action) {
            return binding.chord;
        }
    }
    return std::nullopt;
}

std::optional<domain::Error> ValidateSettings(const Settings& settings) {
    for (size_t index = 0; index < settings.hotkeys.size(); ++index) {
        const HotkeyBinding& binding = settings.hotkeys[index];
        if (const auto valid = domain::ValidateKeyChord(binding.chord); !valid.ok()) {
            return valid.error();
        }
        for (size_t earlier = 0; earlier < index; ++earlier) {
            if (settings.hotkeys[earlier].action == binding.action) {
                return SettingsError(L"An action has more than one shortcut.");
            }
            if (settings.hotkeys[earlier].chord == binding.chord) {
                return SettingsError(domain::FormatKeyChord(binding.chord) +
                                     L" is assigned to more than one action.");
            }
        }
    }
    return std::nullopt;
}

std::wstring SerializeSettings(const Settings& settings) {
    std::wstring text(kHeader);
    text.append(L"\n").append(kHotkeysKey).append(settings.hotkeysEnabled ? L"=on\n" : L"=off\n");
    for (const ActionKind action : kAllActions) {
        const auto chord = settings.ChordFor(action);
        text.append(ToWireName(action)).append(L"=");
        text.append(chord ? domain::FormatKeyChord(*chord) : std::wstring(kNoChord)).append(L"\n");
    }
    return text;
}

domain::Result<Settings> ParseSettings(std::wstring_view text) {
    if (TakeLine(text) != kHeader) {
        return SettingsError(L"Unknown settings header or version.");
    }
    Settings settings;
    while (!text.empty()) {
        const std::wstring_view line = TakeLine(text);
        if (line.empty()) {
            continue;
        }
        const size_t equals = line.find(L'=');
        if (equals == std::wstring_view::npos) {
            return SettingsError(L"Malformed settings line.");
        }
        const std::wstring_view key = line.substr(0, equals);
        const std::wstring_view value = line.substr(equals + 1);

        if (key == kHotkeysKey) {
            if (value != L"on" && value != L"off") {
                return SettingsError(L"hotkeys must be on or off.");
            }
            settings.hotkeysEnabled = value == L"on";
            continue;
        }
        const auto action = ParseActionKind(key);
        if (!action.ok()) {
            return SettingsError(L"Unknown setting: " + std::wstring(key));
        }
        if (value == kNoChord) {
            continue;
        }
        const auto chord = domain::ParseKeyChord(value);
        if (!chord.ok()) {
            return chord.error();
        }
        settings.hotkeys.push_back({action.value(), chord.value()});
    }
    if (const auto invalid = ValidateSettings(settings)) {
        return *invalid;
    }
    return settings;
}

}  // namespace et::app
