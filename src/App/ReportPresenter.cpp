#include "App/ReportPresenter.h"

#include <windows.h>

#include "Domain/PathText.h"
#include "Domain/ProductInfo.h"

namespace et::ui {

namespace {

using domain::ItemStatus;

constexpr size_t kMaxItemsListed = 8;

bool IsProblem(const domain::ItemOutcome& item) {
    return item.status == ItemStatus::Failed || item.status == ItemStatus::Skipped;
}

}  // namespace

std::wstring DescribeProblems(const domain::OperationReport& report) {
    const size_t failed = report.Count(ItemStatus::Failed) + report.Count(ItemStatus::Skipped);
    if (failed == 0) {
        return {};
    }

    std::wstring text = std::to_wstring(report.Count(ItemStatus::Succeeded)) + L" of " +
                        std::to_wstring(report.items.size()) + L" items were completed.\n";
    size_t listed = 0;
    for (const domain::ItemOutcome& item : report.items) {
        if (!IsProblem(item)) {
            continue;
        }
        if (listed == kMaxItemsListed) {
            text += L"\n... and " + std::to_wstring(failed - listed) + L" more.";
            break;
        }
        text += L"\n" + std::wstring(domain::NameOf(item.source)) + L": " + item.detail;
        ++listed;
    }
    const size_t notAttempted = report.Count(ItemStatus::NotAttempted);
    if (notAttempted != 0) {
        text += L"\n\n" + std::to_wstring(notAttempted) + L" items were not attempted.";
    }
    return text;
}

void ShowProblem(const std::wstring& message) {
    MessageBoxW(nullptr, message.c_str(), std::wstring(domain::ProductName()).c_str(),
                MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
}

}  // namespace et::ui
