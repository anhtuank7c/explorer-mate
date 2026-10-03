#include "Domain/KeyChord.h"

#include <optional>
#include <utility>

namespace et::domain {

namespace {

// Windows virtual-key codes. Letters and digits equal their ASCII codes.
constexpr unsigned kFirstFunctionKey = 0x70;  // VK_F1
constexpr unsigned kFunctionKeyCount = 24;

Error ChordError(std::wstring message) {
    return Error(ErrorCode::InvalidArgument, std::move(message));
}

wchar_t ToUpperAscii(wchar_t character) {
    return (character >= L'a' && character <= L'z') ? static_cast<wchar_t>(character - 32)
                                                    : character;
}

std::wstring UpperAscii(std::wstring_view text) {
    std::wstring upper(text);
    for (wchar_t& character : upper) {
        character = ToUpperAscii(character);
    }
    return upper;
}

bool IsLetterOrDigitKey(unsigned key) {
    return (key >= L'A' && key <= L'Z') || (key >= L'0' && key <= L'9');
}

bool IsFunctionKey(unsigned key) {
    return key >= kFirstFunctionKey && key < kFirstFunctionKey + kFunctionKeyCount;
}

// "N" -> 'N', "7" -> '7', "F5" -> VK_F5. nullopt for anything else.
std::optional<unsigned> ParseKeyName(const std::wstring& upperName) {
    if (upperName.size() == 1 && IsLetterOrDigitKey(upperName[0])) {
        return static_cast<unsigned>(upperName[0]);
    }
    if (upperName.size() >= 2 && upperName.size() <= 3 && upperName[0] == L'F') {
        unsigned number = 0;
        for (size_t index = 1; index < upperName.size(); ++index) {
            if (upperName[index] < L'0' || upperName[index] > L'9') {
                return std::nullopt;
            }
            number = number * 10 + static_cast<unsigned>(upperName[index] - L'0');
        }
        if (number >= 1 && number <= kFunctionKeyCount && upperName[1] != L'0') {
            return kFirstFunctionKey + number - 1;
        }
    }
    return std::nullopt;
}

std::wstring KeyName(unsigned key) {
    if (IsFunctionKey(key)) {
        return L"F" + std::to_wstring(key - kFirstFunctionKey + 1);
    }
    return std::wstring(1, static_cast<wchar_t>(key));
}

}  // namespace

std::wstring FormatKeyChord(const KeyChord& chord) {
    std::wstring text;
    if (chord.ctrl) text += L"Ctrl+";
    if (chord.alt) text += L"Alt+";
    if (chord.shift) text += L"Shift+";
    if (chord.win) text += L"Win+";
    return text + KeyName(chord.key);
}

Result<KeyChord> ValidateKeyChord(const KeyChord& chord) {
    if (!IsLetterOrDigitKey(chord.key) && !IsFunctionKey(chord.key)) {
        return ChordError(L"The shortcut key must be a letter, a digit or F1 to F24.");
    }
    if (!chord.ctrl && !chord.alt && !chord.win) {
        return ChordError(L"The shortcut must include Ctrl, Alt or Win.");
    }
    return chord;
}

Result<KeyChord> ParseKeyChord(std::wstring_view text) {
    const std::wstring original(text);
    KeyChord chord;
    bool hasKey = false;
    while (true) {
        const size_t plus = text.find(L'+');
        const std::wstring part = UpperAscii(text.substr(0, plus));
        const bool isLast = plus == std::wstring_view::npos;

        if (part == L"CTRL" && !isLast) {
            chord.ctrl = true;
        } else if (part == L"ALT" && !isLast) {
            chord.alt = true;
        } else if (part == L"SHIFT" && !isLast) {
            chord.shift = true;
        } else if (part == L"WIN" && !isLast) {
            chord.win = true;
        } else if (const auto key = ParseKeyName(part); key && isLast) {
            chord.key = *key;
            hasKey = true;
        } else {
            return ChordError(L"Not a valid shortcut: " + original);
        }
        if (isLast) {
            break;
        }
        text.remove_prefix(plus + 1);
    }
    if (!hasKey) {
        return ChordError(L"The shortcut has no key.");
    }
    return ValidateKeyChord(chord);
}

}  // namespace et::domain
