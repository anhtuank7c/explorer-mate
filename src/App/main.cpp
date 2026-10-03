#include <windows.h>

#include <string>
#include <vector>

#include "App/AboutDialog.h"
#include "App/ActionRunner.h"
#include "App/Agent.h"
#include "App/CommandLine.h"
#include "App/DocumentDialog.h"
#include "App/DialogPrompt.h"
#include "App/ExplorerDiagnostics.h"
#include "App/Options.h"
#include "App/PresetPrompt.h"
#include "App/ReportPresenter.h"
#include "Domain/ProductInfo.h"
#include "Infrastructure/AppDataPaths.h"
#include "Infrastructure/ComApartment.h"
#include "Infrastructure/RequestFileStore.h"

namespace {

using et::domain::ItemStatus;

constexpr int kExitOk = 0;
constexpr int kExitFailed = 1;      // An error, or at least one item not completed.
constexpr int kExitUsage = 2;
constexpr int kExitCancelled = 3;
constexpr unsigned kStaleRequestSeconds = 24 * 60 * 60;

const wchar_t* StatusLabel(ItemStatus status) {
    switch (status) {
        case ItemStatus::Succeeded:
            return L"OK";
        case ItemStatus::Failed:
            return L"FAILED";
        case ItemStatus::Skipped:
            return L"SKIPPED";
        case ItemStatus::NotAttempted:
            return L"NOT ATTEMPTED";
    }
    return L"";
}

void PrintReport(const et::domain::OperationReport& report) {
    for (const et::domain::ItemOutcome& item : report.items) {
        std::wstring line = std::wstring(StatusLabel(item.status)) + L"\t" + item.source;
        if (!item.destination.empty()) {
            line += L"\t-> " + item.destination;
        }
        if (!item.detail.empty()) {
            line += L"\t" + item.detail;
        }
        et::ui::WriteLineToStdout(line);
    }
}

et::domain::Result<et::app::ActionRequest> LoadRequest(const et::ui::Options& options) {
    if (!options.requestFile) {
        std::vector<std::wstring> items;
        for (const std::wstring& item : options.items) {
            items.push_back(et::ui::AbsoluteItemPath(item));
        }
        return et::app::ActionRequest{*options.action, std::move(items)};
    }
    const auto dataDirectory = et::infra::ProductDataDirectory();
    if (!dataDirectory.ok()) {
        return dataDirectory.error();
    }
    const et::infra::RequestFileStore store(dataDirectory.value() + L"\\requests");
    auto request = store.Take(*options.requestFile);
    store.RemoveStale(kStaleRequestSeconds);
    return request;
}

int Run(const std::vector<std::wstring>& arguments) {
    const auto options = et::ui::ParseOptions(arguments);
    if (!options.ok()) {
        et::ui::WriteLineToStdout(options.error().message);
        et::ui::WriteLineToStdout(L"Run with --help to see the command line.");
        return kExitUsage;
    }
    if (options.value().showHelp) {
        et::ui::WriteLineToStdout(et::ui::UsageText());
        return kExitOk;
    }
    if (options.value().showVersion) {
        et::ui::WriteLineToStdout(std::wstring(et::domain::ProductName()) + L" " +
                                  std::wstring(et::domain::ProductVersion()));
        return kExitOk;
    }

    // Asked for on the command line, these stay in the terminal; the windows with the same
    // content are reached from the introduction window and the tray menu.
    if (options.value().showChangelog) {
        et::ui::WriteLineToStdout(et::ui::ChangelogText());
        return kExitOk;
    }
    if (options.value().showLicenses) {
        et::ui::WriteLineToStdout(et::ui::LicensesText());
        return kExitOk;
    }
    if (options.value().showAbout) {
        et::ui::WriteLineToStdout(et::ui::AboutText());
        return kExitOk;
    }
    if (options.value().showIntroduction) {
        et::ui::ShowAbout();
        return kExitOk;
    }
    if (options.value().stopAgent) {
        et::ui::WriteLineToStdout(et::ui::StopRunningAgent() ? L"Agent stopped."
                                                             : L"No agent is running.");
        return kExitOk;
    }
    if (options.value().runAgent) {
        const et::infra::ComApartment apartment;
        return et::ui::RunAgent();
    }
    if (options.value().diagnoseExplorer) {
        const et::infra::ComApartment apartment;
        if (options.value().watchSeconds != 0) {
            et::ui::WatchExplorer(options.value().watchSeconds);
        } else {
            et::ui::PrintExplorerDiagnostics(options.value().delaySeconds);
        }
        return kExitOk;
    }

    const auto request = LoadRequest(options.value());
    if (!request.ok()) {
        et::ui::WriteLineToStdout(request.error().message);
        // Started from the menu or a shortcut there is no console: without this the user
        // would click a command and see nothing happen.
        if (options.value().requestFile && !options.value().silent) {
            et::ui::ShowProblem(request.error().message);
        }
        return kExitUsage;
    }

    // Answers given on the command line (or --silent) replace the dialogs entirely.
    const bool interactive = !options.value().silent;
    const bool hasPresetAnswers = options.value().folderName || options.value().renamePattern;
    et::ui::PresetPrompt presetPrompt(options.value().folderName, options.value().renamePattern);
    et::ui::DialogPrompt dialogPrompt;
    et::app::IUserPrompt& prompt = (interactive && !hasPresetAnswers)
                                       ? static_cast<et::app::IUserPrompt&>(dialogPrompt)
                                       : presetPrompt;

    const et::infra::ComApartment apartment;
    const auto report = et::ui::RunAction(request.value(), prompt,
                                          interactive ? et::infra::OperationUi::Interactive
                                                      : et::infra::OperationUi::Silent);
    if (!report.ok()) {
        et::ui::WriteLineToStdout(report.error().message);
        if (report.error().code == et::domain::ErrorCode::Cancelled) {
            return kExitCancelled;
        }
        if (interactive) {
            et::ui::ShowProblem(report.error().message);
        }
        return kExitFailed;
    }

    PrintReport(report.value());
    if (const std::wstring problems = et::ui::DescribeProblems(report.value());
        interactive && !problems.empty()) {
        et::ui::ShowProblem(problems);
    }
    if (report.value().cancelled) {
        return kExitCancelled;
    }
    return report.value().AllSucceeded() ? kExitOk : kExitFailed;
}

}  // namespace

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int) {
    // Last line of defence: an escaped exception (out of memory, a filesystem error) must
    // end the process with an error code, not with a crash dialog.
    try {
        return Run(et::ui::ReadProcessArguments());
    } catch (...) {
        return kExitFailed;
    }
}
