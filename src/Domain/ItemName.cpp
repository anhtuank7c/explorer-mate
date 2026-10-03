#include "Domain/ItemName.h"

#include <algorithm>
#include <array>

namespace et::domain {

namespace {

constexpr size_t kMaxNameLength = 255;
constexpr std::wstring_view kForbiddenCharacters = L"<>:\"/\\|?*";
// COM/LPT also exist with digit 0 and with the superscript digits 1 to 3.
constexpr std::array<std::wstring_view, 32> kReservedDeviceNames{
    L"CON",      L"PRN",      L"AUX",      L"NUL",      L"CONIN$",   L"CONOUT$",
    L"COM0",     L"COM1",     L"COM2",     L"COM3",     L"COM4",     L"COM5",
    L"COM6",     L"COM7",     L"COM8",     L"COM9",     L"COM¹", L"COM²",
    L"COM³", L"LPT0",     L"LPT1",     L"LPT2",     L"LPT3",     L"LPT4",
    L"LPT5",     L"LPT6",     L"LPT7",     L"LPT8",     L"LPT9",     L"LPT¹",
    L"LPT²", L"LPT³"};

Error InvalidName(const wchar_t* reason) {
    return Error(ErrorCode::InvalidName, reason);
}

bool IsControlCharacter(wchar_t character) {
    return character < 0x20;
}

// Windows reserves device names with or without an extension: "NUL" and "nul.txt" alike.
bool IsReservedDeviceName(std::wstring_view name) {
    std::wstring base(name.substr(0, name.find(L'.')));
    while (!base.empty() && base.back() == L' ') {
        base.pop_back();
    }
    std::transform(base.begin(), base.end(), base.begin(), [](wchar_t character) {
        return (character >= L'a' && character <= L'z')
                   ? static_cast<wchar_t>(character - (L'a' - L'A'))
                   : character;
    });
    return std::find(kReservedDeviceNames.begin(), kReservedDeviceNames.end(), base) !=
           kReservedDeviceNames.end();
}

}  // namespace

NameParts SplitStemAndExtension(std::wstring_view name) {
    const size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring_view::npos || dot == 0) {
        return {std::wstring(name), {}};
    }
    return {std::wstring(name.substr(0, dot)), std::wstring(name.substr(dot))};
}

std::optional<Error> ValidateItemName(std::wstring_view name) {
    if (name.empty()) {
        return InvalidName(L"The name cannot be empty.");
    }
    if (name.size() > kMaxNameLength) {
        return InvalidName(L"The name is too long.");
    }
    if (name.find_first_of(kForbiddenCharacters) != std::wstring_view::npos ||
        std::any_of(name.begin(), name.end(), IsControlCharacter)) {
        return InvalidName(L"The name cannot contain any of these characters: \\ / : * ? \" < > |");
    }
    if (name.back() == L'.' || name.back() == L' ') {
        return InvalidName(L"The name cannot end with a dot or a space.");
    }
    if (name.front() == L' ') {
        return InvalidName(L"The name cannot start with a space.");
    }
    if (IsReservedDeviceName(name)) {
        return InvalidName(L"This name is reserved by Windows.");
    }
    return std::nullopt;
}

}  // namespace et::domain
