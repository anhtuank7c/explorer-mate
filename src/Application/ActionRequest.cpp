#include "Application/ActionRequest.h"

#include <optional>
#include <utility>

#include "Domain/PathText.h"

namespace et::app {

namespace {

constexpr std::wstring_view kHeader = L"ExplorerMate-Request 1";
constexpr std::wstring_view kActionKey = L"action=";
constexpr std::wstring_view kItemKey = L"item=";
// The duplicate check and rename planning compare items pairwise; beyond this the worker
// would appear to hang. Far above any selection made by hand.
constexpr size_t kMaxItems = 5000;

domain::Error Malformed(const wchar_t* reason) {
    return domain::Error(domain::ErrorCode::InvalidArgument,
                         std::wstring(L"Invalid request: ") + reason);
}

// Returns the next line without its terminator and advances `text` past it.
std::wstring_view TakeLine(std::wstring_view& text) {
    const size_t end = text.find(L'\n');
    std::wstring_view line = text.substr(0, end);
    text.remove_prefix(end == std::wstring_view::npos ? text.size() : end + 1);
    if (!line.empty() && line.back() == L'\r') {
        line.remove_suffix(1);
    }
    return line;
}

bool StartsWith(std::wstring_view text, std::wstring_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}

}  // namespace

std::wstring SerializeRequest(const ActionRequest& request) {
    std::wstring text(kHeader);
    text.append(L"\n").append(kActionKey).append(ToWireName(request.action)).append(L"\n");
    for (const std::wstring& item : request.items) {
        text.append(kItemKey).append(item).append(L"\n");
    }
    return text;
}

domain::Result<ActionRequest> ParseRequest(std::wstring_view text) {
    if (TakeLine(text) != kHeader) {
        return Malformed(L"unknown header or version.");
    }

    std::optional<ActionKind> action;
    std::vector<std::wstring> items;
    while (!text.empty()) {
        const std::wstring_view line = TakeLine(text);
        if (line.empty()) {
            continue;
        }
        if (StartsWith(line, kActionKey)) {
            if (action) {
                return Malformed(L"more than one action.");
            }
            const auto parsed = ParseActionKind(line.substr(kActionKey.size()));
            if (!parsed.ok()) {
                return parsed.error();
            }
            action = parsed.value();
        } else if (StartsWith(line, kItemKey)) {
            const std::wstring_view path = line.substr(kItemKey.size());
            if (!domain::IsDriveAbsoluteItemPath(path)) {
                // The common real cause is a network or virtual location; say so plainly,
                // because this message is shown to the user.
                return domain::Error(domain::ErrorCode::UnsupportedLocation,
                                     L"Only items in a regular folder on a local drive are "
                                     L"supported. This one is not: " + std::wstring(path));
            }
            if (items.size() == kMaxItems) {
                return domain::Error(domain::ErrorCode::InvalidArgument,
                                     L"Too many items are selected. The limit is " +
                                         std::to_wstring(kMaxItems) + L".");
            }
            items.emplace_back(path);
        } else {
            return Malformed(L"unknown line.");
        }
    }

    if (!action) {
        return Malformed(L"no action.");
    }
    if (items.empty()) {
        return Malformed(L"no items.");
    }
    return ActionRequest{*action, std::move(items)};
}

}  // namespace et::app
