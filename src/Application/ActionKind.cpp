#include "Application/ActionKind.h"

#include <array>
#include <string>
#include <utility>

namespace et::app {

namespace {

constexpr std::array<std::pair<ActionKind, std::wstring_view>, 3> kWireNames{{
    {ActionKind::GroupIntoNewFolder, L"group"},
    {ActionKind::BulkRename, L"rename"},
    {ActionKind::DuplicateInPlace, L"duplicate"},
}};

}  // namespace

std::wstring_view ToWireName(ActionKind action) {
    for (const auto& [kind, name] : kWireNames) {
        if (kind == action) {
            return name;
        }
    }
    return {};
}

domain::Result<ActionKind> ParseActionKind(std::wstring_view wireName) {
    for (const auto& [kind, name] : kWireNames) {
        if (name == wireName) {
            return kind;
        }
    }
    return domain::Error(domain::ErrorCode::InvalidArgument,
                         L"Unknown action: " + std::wstring(wireName));
}

}  // namespace et::app
