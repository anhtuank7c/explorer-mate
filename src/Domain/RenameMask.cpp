#include "Domain/RenameMask.h"

#include <algorithm>
#include <utility>

namespace et::domain {

namespace {

constexpr std::wstring_view kLiteralOpen = L"[[]";
constexpr std::wstring_view kLiteralClose = L"[]]";
constexpr size_t kMaxPosition = 100000;

Error MaskError(std::wstring message) {
    return Error(ErrorCode::InvalidArgument, std::move(message));
}

bool IsDigit(wchar_t character) {
    return character >= L'0' && character <= L'9';
}

// Reads a positive number at `position` and advances past it. nullopt when there is none
// or it is zero or absurdly large.
std::optional<size_t> TakeNumber(std::wstring_view text, size_t& position) {
    size_t value = 0;
    const size_t begin = position;
    while (position < text.size() && IsDigit(text[position])) {
        value = value * 10 + static_cast<size_t>(text[position] - L'0');
        if (value > kMaxPosition) {
            return std::nullopt;
        }
        ++position;
    }
    return (position == begin || value == 0) ? std::nullopt : std::optional<size_t>(value);
}

// Applies the range part of a placeholder ("", "2", "2-5", "2-", "2,3") to `source`.
std::optional<std::wstring_view> ApplyRange(std::wstring_view source, std::wstring_view range) {
    if (range.empty()) {
        return source;
    }
    size_t position = 0;
    const auto first = TakeNumber(range, position);
    if (!first) {
        return std::nullopt;
    }
    const size_t start = std::min(*first - 1, source.size());
    if (position == range.size()) {
        return source.substr(start, 1);
    }

    const wchar_t separator = range[position++];
    if (separator == L'-' && position == range.size()) {
        return source.substr(start);
    }
    const auto second = TakeNumber(range, position);
    if (!second || position != range.size()) {
        return std::nullopt;
    }
    if (separator == L',') {
        return source.substr(start, *second);
    }
    if (separator == L'-' && *second >= *first) {
        return source.substr(start, *second - *first + 1);
    }
    return std::nullopt;
}

// Resolves the text between "[" and "]". nullopt when it is not a known placeholder.
std::optional<std::wstring> Resolve(std::wstring_view placeholder, const MaskContext& context) {
    if (placeholder.empty()) {
        return std::nullopt;
    }
    if (placeholder == L"C") {
        return std::wstring(context.counter);
    }
    std::wstring_view source;
    switch (placeholder.front()) {
        case L'N':
            source = context.name;
            break;
        case L'E':
            source = context.extension;
            break;
        case L'P':
            source = context.parent;
            break;
        default:
            return std::nullopt;
    }
    const auto part = ApplyRange(source, placeholder.substr(1));
    return part ? std::optional<std::wstring>(std::wstring(*part)) : std::nullopt;
}

}  // namespace

Result<std::wstring> ExpandMask(std::wstring_view mask, const MaskContext& context) {
    std::wstring expanded;
    for (size_t position = 0; position < mask.size();) {
        if (mask.substr(position, kLiteralOpen.size()) == kLiteralOpen) {
            expanded.push_back(L'[');
            position += kLiteralOpen.size();
            continue;
        }
        if (mask.substr(position, kLiteralClose.size()) == kLiteralClose) {
            expanded.push_back(L']');
            position += kLiteralClose.size();
            continue;
        }
        if (mask[position] == L']') {
            return MaskError(L"\"]\" without a matching \"[\". Use []] for a literal bracket.");
        }
        if (mask[position] != L'[') {
            expanded.push_back(mask[position++]);
            continue;
        }

        const size_t close = mask.find(L']', position);
        if (close == std::wstring_view::npos) {
            return MaskError(L"\"[\" is not closed. Use [[] for a literal bracket.");
        }
        const std::wstring_view placeholder = mask.substr(position + 1, close - position - 1);
        const auto value = Resolve(placeholder, context);
        if (!value) {
            return MaskError(L"Unknown placeholder [" + std::wstring(placeholder) + L"].");
        }
        expanded += *value;
        position = close + 1;
    }
    return expanded;
}

std::optional<Error> ValidateMask(std::wstring_view mask) {
    const auto expanded = ExpandMask(mask, MaskContext{});
    return expanded.ok() ? std::nullopt : std::optional<Error>(expanded.error());
}

}  // namespace et::domain
