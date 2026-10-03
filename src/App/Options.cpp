#include "App/Options.h"

#include <cwchar>

namespace et::ui {

namespace {

domain::Error UsageError(const std::wstring& message) {
    return domain::Error(domain::ErrorCode::InvalidArgument, message);
}

std::optional<unsigned> ParseNumber(const std::wstring& text) {
    if (text.empty() || text.size() > 7 ||
        text.find_first_not_of(L"0123456789") != std::wstring::npos) {
        return std::nullopt;
    }
    return static_cast<unsigned>(std::wcstoul(text.c_str(), nullptr, 10));
}

bool IsOption(const std::wstring& argument) {
    return argument.rfind(L"--", 0) == 0;
}

}  // namespace

domain::Result<Options> ParseOptions(const std::vector<std::wstring>& arguments) {
    Options options;
    // Started by double-click, with nothing to do: introduce the program instead.
    options.showAbout = arguments.empty();
    const auto pattern = [&]() -> domain::RenamePattern& {
        if (!options.renamePattern) {
            options.renamePattern = domain::RenamePattern{};
        }
        return *options.renamePattern;
    };

    for (size_t index = 0; index < arguments.size(); ++index) {
        const std::wstring& argument = arguments[index];
        if (!IsOption(argument)) {
            options.items.push_back(argument);
            continue;
        }
        if (argument == L"--version") {
            options.showVersion = true;
            continue;
        }
        if (argument == L"--silent") {
            options.silent = true;
            continue;
        }
        if (argument == L"--about") {
            options.showAbout = true;
            continue;
        }
        if (argument == L"--changelog") {
            options.showChangelog = true;
            continue;
        }
        if (argument == L"--licenses") {
            options.showLicenses = true;
            continue;
        }
        if (argument == L"--agent") {
            options.runAgent = true;
            continue;
        }
        if (argument == L"--stop-agent") {
            options.stopAgent = true;
            continue;
        }
        if (argument == L"--diagnose-explorer") {
            options.diagnoseExplorer = true;
            continue;
        }

        if (index + 1 == arguments.size()) {
            return UsageError(L"Missing value for " + argument);
        }
        const std::wstring& value = arguments[++index];
        if (argument == L"--request") {
            options.requestFile = value;
        } else if (argument == L"--action") {
            const auto action = app::ParseActionKind(value);
            if (!action.ok()) {
                return action.error();
            }
            options.action = action.value();
        } else if (argument == L"--name") {
            options.folderName = value;
        } else if (argument == L"--delay") {
            const auto seconds = ParseNumber(value);
            if (!seconds || *seconds > 60) {
                return UsageError(L"--delay takes 0 to 60 seconds.");
            }
            options.delaySeconds = *seconds;
        } else if (argument == L"--watch") {
            const auto seconds = ParseNumber(value);
            if (!seconds || *seconds == 0 || *seconds > 600) {
                return UsageError(L"--watch takes 1 to 600 seconds.");
            }
            options.watchSeconds = *seconds;
        } else if (argument == L"--mask") {
            pattern().nameMask = value;
        } else if (argument == L"--ext-mask") {
            pattern().extensionMask = value;
        } else if (argument == L"--search") {
            pattern().searchFor = value;
        } else if (argument == L"--replace") {
            pattern().replaceWith = value;
        } else if (argument == L"--start" || argument == L"--step" || argument == L"--digits") {
            const auto number = ParseNumber(value);
            if (!number) {
                return UsageError(L"Not a number: " + value);
            }
            unsigned& field = argument == L"--start"  ? pattern().counterStart
                              : argument == L"--step" ? pattern().counterStep
                                                      : pattern().counterDigits;
            field = *number;
        } else {
            return UsageError(L"Unknown option: " + argument);
        }
    }

    if (options.showVersion || options.showAbout || options.showChangelog || options.showLicenses ||
        options.diagnoseExplorer || options.runAgent ||
        options.stopAgent) {
        return options;
    }
    const bool hasRequest = options.requestFile.has_value();
    const bool hasDirectAction = options.action.has_value();
    if (hasRequest == hasDirectAction) {
        return UsageError(L"Use either --request <file> or --action <name> <item>...");
    }
    if (hasRequest && !options.items.empty()) {
        return UsageError(L"--request does not take items.");
    }
    if (hasDirectAction && options.items.empty()) {
        return UsageError(L"No items given.");
    }
    return options;
}

}  // namespace et::ui
