#include "Infrastructure/ShellFileOperationGateway.h"

#include <windows.h>

#include <shellapi.h>
#include <sherrors.h>
#include <shlobj.h>
#include <shobjidl_core.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include <algorithm>
#include <functional>
#include <optional>
#include <utility>

#include "Domain/PathText.h"
#include "Infrastructure/ErrorText.h"

namespace et::infra {

namespace {

using domain::ItemOutcome;
using domain::ItemStatus;
using domain::OperationReport;
using Microsoft::WRL::ComPtr;

bool SamePath(std::wstring_view left, std::wstring_view right) {
    return CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(),
                                static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

std::wstring FileSystemPath(IShellItem* item) {
    if (item == nullptr) {
        return {};
    }
    PWSTR raw = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw))) {
        return {};
    }
    std::wstring path(raw);
    CoTaskMemFree(raw);
    return path;
}

// The Shell reports long paths; callers may pass 8.3 short ones (%TEMP% often is). Must be
// called while the item still exists. Returns the input unchanged when it cannot be expanded.
std::wstring LongPath(const std::wstring& path) {
    const DWORD needed = GetLongPathNameW(path.c_str(), nullptr, 0);
    if (needed == 0) {
        return path;
    }
    std::wstring expanded(needed, L'\0');
    const DWORD written = GetLongPathNameW(path.c_str(), expanded.data(), needed);
    if (written == 0 || written >= needed) {
        return path;
    }
    expanded.resize(written);
    return expanded;
}

ComPtr<IShellItem> ShellItemFor(const std::wstring& path, HRESULT& result) {
    ComPtr<IShellItem> item;
    result = SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&item));
    return item;
}

bool IsCancellation(HRESULT result) {
    return result == COPYENGINE_E_USER_CANCELLED || result == HRESULT_FROM_WIN32(ERROR_CANCELLED) ||
           result == E_ABORT;
}

struct CompletedItem {
    std::wstring source;
    std::wstring destination;
    HRESULT result = E_FAIL;
};

// Collects what the Shell actually did. Destination names come from here, never from a
// guess, because the Shell picks the name when it resolves a collision.
class ProgressSink final
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          IFileOperationProgressSink> {
public:
    explicit ProgressSink(bool stopAfterFirstFailure)
        : stopAfterFirstFailure_(stopAfterFirstFailure) {}

    const std::vector<CompletedItem>& completed() const { return completed_; }

    IFACEMETHODIMP StartOperations() override { return S_OK; }
    IFACEMETHODIMP FinishOperations(HRESULT) override { return S_OK; }
    IFACEMETHODIMP PreRenameItem(DWORD, IShellItem*, LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP PostRenameItem(DWORD, IShellItem* item, LPCWSTR, HRESULT result,
                                  IShellItem* created) override {
        return Record(item, result, created);
    }
    IFACEMETHODIMP PreMoveItem(DWORD, IShellItem*, IShellItem*, LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP PostMoveItem(DWORD, IShellItem* item, IShellItem*, LPCWSTR, HRESULT result,
                                IShellItem* created) override {
        return Record(item, result, created);
    }
    IFACEMETHODIMP PreCopyItem(DWORD, IShellItem*, IShellItem*, LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP PostCopyItem(DWORD, IShellItem* item, IShellItem*, LPCWSTR, HRESULT result,
                                IShellItem* created) override {
        return Record(item, result, created);
    }
    IFACEMETHODIMP PreDeleteItem(DWORD, IShellItem*) override { return S_OK; }
    IFACEMETHODIMP PostDeleteItem(DWORD, IShellItem*, HRESULT, IShellItem*) override {
        return S_OK;
    }
    IFACEMETHODIMP PreNewItem(DWORD, IShellItem*, LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP PostNewItem(DWORD, IShellItem*, LPCWSTR, LPCWSTR, DWORD, HRESULT,
                               IShellItem*) override {
        return S_OK;
    }
    IFACEMETHODIMP UpdateProgress(UINT, UINT) override { return S_OK; }
    IFACEMETHODIMP ResetTimer() override { return S_OK; }
    IFACEMETHODIMP PauseTimer() override { return S_OK; }
    IFACEMETHODIMP ResumeTimer() override { return S_OK; }

private:
    // Returning a failure from a Post* callback cancels every operation still pending,
    // which is how dependent rename steps are stopped after the first step that did not
    // complete. "Did not complete" includes success codes without a resulting item, such as
    // the user choosing Skip at a conflict prompt: a later step may have been planned on the
    // assumption that this one vacated its name.
    HRESULT Record(IShellItem* item, HRESULT result, IShellItem* created) {
        try {
            completed_.push_back({FileSystemPath(item), FileSystemPath(created), result});
        } catch (...) {
            return E_OUTOFMEMORY;
        }
        const bool completed = SUCCEEDED(result) && result != COPYENGINE_S_USER_IGNORED &&
                               created != nullptr;
        return (stopAfterFirstFailure_ && !completed) ? E_ABORT : S_OK;
    }

    bool stopAfterFirstFailure_;
    std::vector<CompletedItem> completed_;
};

struct BatchOptions {
    DWORD extraFlags = 0;
    bool stopAfterFirstFailure = false;
};

// Queues one operation for `item`, the shell item of sources[index].
using QueueOperation = std::function<HRESULT(IFileOperation&, IShellItem*, size_t index)>;

DWORD OperationFlags(OperationUi ui, DWORD extraFlags) {
    // FOF_NOCONFIRMATION is deliberately absent: it would answer "yes" to overwrite prompts.
    DWORD flags = FOF_NOCONFIRMMKDIR | extraFlags;
    if (ui == OperationUi::Silent) {
        // No undo record: silent runs (tests, scripts) must not fill Explorer's Ctrl+Z stack.
        return flags | FOF_SILENT | FOF_NOERRORUI;
    }
    return flags | FOF_ALLOWUNDO | FOFX_ADDUNDORECORD;
}

ItemOutcome FailedOutcome(const std::wstring& source, HRESULT result) {
    return {source, {}, ItemStatus::Failed, DescribeHresult(result)};
}

ItemOutcome OutcomeFrom(const std::wstring& source, const CompletedItem& completed) {
    if (IsCancellation(completed.result)) {
        return {source, {}, ItemStatus::NotAttempted, {}};
    }
    if (FAILED(completed.result)) {
        return FailedOutcome(source, completed.result);
    }
    // Other success codes are normal: moves and renames end with
    // COPYENGINE_S_DONT_PROCESS_CHILDREN because the whole item was handled in one go.
    if (completed.result == COPYENGINE_S_USER_IGNORED) {
        return {source, {}, ItemStatus::Skipped, L"Skipped."};
    }
    // A success code without a resulting item means the Shell left the item alone, e.g.
    // because a conflict prompt was answered with "skip".
    if (completed.destination.empty()) {
        return {source, {}, ItemStatus::Skipped, L"The item was left unchanged."};
    }
    return {source, completed.destination, ItemStatus::Succeeded, {}};
}

OperationReport AllFailed(const std::vector<std::wstring>& sources, HRESULT result) {
    OperationReport report;
    for (const std::wstring& source : sources) {
        report.items.push_back(FailedOutcome(source, result));
    }
    return report;
}

OperationReport RunBatch(OperationUi ui, const std::vector<std::wstring>& sources,
                         const BatchOptions& options, const QueueOperation& queue) {
    ComPtr<IFileOperation> operation;
    HRESULT result =
        CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&operation));
    if (SUCCEEDED(result)) {
        result = operation->SetOperationFlags(OperationFlags(ui, options.extraFlags));
    }
    const ComPtr<ProgressSink> sink = Microsoft::WRL::Make<ProgressSink>(options.stopAfterFirstFailure);
    DWORD cookie = 0;
    if (SUCCEEDED(result)) {
        result = sink ? operation->Advise(sink.Get(), &cookie) : E_OUTOFMEMORY;
    }
    if (FAILED(result)) {
        return AllFailed(sources, result);
    }

    // Items that could not even be queued keep their own failure reason.
    std::vector<std::optional<HRESULT>> queueFailures(sources.size());
    std::vector<std::wstring> reportedPaths(sources.size());
    bool anyQueued = false;
    for (size_t index = 0; index < sources.size(); ++index) {
        reportedPaths[index] = LongPath(sources[index]);
        HRESULT itemResult = S_OK;
        const ComPtr<IShellItem> item = ShellItemFor(sources[index], itemResult);
        if (SUCCEEDED(itemResult)) {
            itemResult = queue(*operation.Get(), item.Get(), index);
        }
        if (FAILED(itemResult)) {
            queueFailures[index] = itemResult;
            if (options.stopAfterFirstFailure) {
                break;
            }
            continue;
        }
        anyQueued = true;
    }

    const bool queueBroken =
        options.stopAfterFirstFailure &&
        std::any_of(queueFailures.begin(), queueFailures.end(),
                    [](const std::optional<HRESULT>& failure) { return failure.has_value(); });

    BOOL aborted = FALSE;
    if (anyQueued && !queueBroken) {
        const HRESULT performed = operation->PerformOperations();
        operation->GetAnyOperationsAborted(&aborted);
        aborted = aborted || IsCancellation(performed);
    }
    operation->Unadvise(cookie);

    OperationReport report;
    report.cancelled = aborted != FALSE && !options.stopAfterFirstFailure;
    for (size_t index = 0; index < sources.size(); ++index) {
        const std::wstring& source = sources[index];
        if (queueFailures[index]) {
            report.items.push_back(FailedOutcome(source, *queueFailures[index]));
            continue;
        }
        const auto& completed = sink->completed();
        const auto found = std::find_if(completed.begin(), completed.end(),
                                        [&](const CompletedItem& item) {
                                            return SamePath(item.source, reportedPaths[index]);
                                        });
        report.items.push_back(found != completed.end()
                                   ? OutcomeFrom(source, *found)
                                   : ItemOutcome{source, {}, ItemStatus::NotAttempted, {}});
    }
    return report;
}

bool HasProblem(const OperationReport& report) {
    return report.cancelled || report.Count(ItemStatus::Succeeded) != report.items.size();
}

// A step whose source is produced by an earlier step of the same batch cannot be queued
// yet, because its shell item does not exist until that earlier step has run.
size_t SegmentEnd(const std::vector<domain::RenameStep>& steps, size_t begin) {
    size_t end = begin + 1;
    for (; end < steps.size(); ++end) {
        const bool dependsOnSegment =
            std::any_of(steps.begin() + begin, steps.begin() + end,
                        [&](const domain::RenameStep& earlier) {
                            return SamePath(earlier.to, steps[end].from);
                        });
        if (dependsOnSegment) {
            break;
        }
    }
    return end;
}

// Tells open Explorer windows that the contents of `folder` changed, and waits until the
// notification has been delivered. The worker exits right after its operation; shell change
// notifications are queued in the sending process, so without the flush they are lost and
// the file list keeps showing the old contents until the user presses F5.
void NotifyFolderChanged(const std::wstring& folder) {
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW | SHCNF_FLUSH, folder.c_str(), nullptr);
}

void NotifyParentsChanged(const std::vector<std::wstring>& paths) {
    std::vector<std::wstring> notified;
    for (const std::wstring& path : paths) {
        std::wstring parent(domain::ParentOf(path));
        const bool alreadyDone = std::any_of(notified.begin(), notified.end(),
                                             [&](const std::wstring& done) { return SamePath(done, parent); });
        if (!alreadyDone) {
            NotifyFolderChanged(parent);
            notified.push_back(std::move(parent));
        }
    }
}

bool ExistsOnDisk(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// First step in [begin, end) whose target is occupied by something no earlier step of the
// segment moves away; `end` when there is none. Checked here because the Shell would answer
// an occupied target with a replace/skip prompt, even when asked to be silent.
size_t FirstBlockedStep(const std::wstring& folder, const std::vector<domain::RenameStep>& steps,
                        size_t begin, size_t end) {
    for (size_t index = begin; index < end; ++index) {
        const domain::RenameStep& step = steps[index];
        const bool caseOnlyChange = SamePath(step.from, step.to);
        const bool freedEarlier =
            std::any_of(steps.begin() + begin, steps.begin() + index,
                        [&](const domain::RenameStep& earlier) {
                            return SamePath(earlier.from, step.to);
                        });
        if (!caseOnlyChange && !freedEarlier && ExistsOnDisk(domain::JoinPath(folder, step.to))) {
            return index;
        }
    }
    return end;
}

}  // namespace

ShellFileOperationGateway::ShellFileOperationGateway(OperationUi ui) : ui_(ui) {}

domain::Status ShellFileOperationGateway::CreateNewFolder(const std::wstring& path) {
    // CreateDirectoryW fails when anything already has this name, which is the ownership
    // guarantee the caller relies on. IFileOperation::NewItem could silently reuse or rename.
    if (CreateDirectoryW(path.c_str(), nullptr)) {
        SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW | SHCNF_FLUSH, path.c_str(), nullptr);
        return domain::Unit{};
    }
    const DWORD error = GetLastError();
    const auto code = (error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS)
                          ? domain::ErrorCode::NameCollision
                          : (error == ERROR_ACCESS_DENIED ? domain::ErrorCode::AccessDenied
                                                          : domain::ErrorCode::OperationFailed);
    return domain::Error(code, L"Cannot create " + path + L": " + DescribeWin32Error(error));
}

domain::Status ShellFileOperationGateway::RemoveFolderIfEmpty(const std::wstring& path) {
    // RemoveDirectoryW refuses non-empty folders, so this can never delete user data.
    if (RemoveDirectoryW(path.c_str())) {
        SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW | SHCNF_FLUSH, path.c_str(), nullptr);
        return domain::Unit{};
    }
    return domain::Error(domain::ErrorCode::OperationFailed,
                         L"Cannot remove " + path + L": " + DescribeWin32Error(GetLastError()));
}

OperationReport ShellFileOperationGateway::MoveItemsInto(const std::vector<std::wstring>& sources,
                                                         const std::wstring& destinationFolder) {
    HRESULT result = S_OK;
    const ComPtr<IShellItem> destination = ShellItemFor(destinationFolder, result);
    if (FAILED(result)) {
        return AllFailed(sources, result);
    }
    OperationReport report =
        RunBatch(ui_, sources, {}, [&](IFileOperation& operation, IShellItem* item, size_t) {
            return operation.MoveItem(item, destination.Get(), nullptr, nullptr);
        });
    NotifyParentsChanged(sources);
    NotifyFolderChanged(destinationFolder);
    return report;
}

OperationReport ShellFileOperationGateway::DuplicateItems(const std::vector<std::wstring>& sources) {
    BatchOptions options;
    options.extraFlags = FOF_RENAMEONCOLLISION;
    OperationReport report = RunBatch(
        ui_, sources, options, [&](IFileOperation& operation, IShellItem* item, size_t index) {
            HRESULT result = S_OK;
            const ComPtr<IShellItem> parent =
                ShellItemFor(std::wstring(domain::ParentOf(sources[index])), result);
            return FAILED(result) ? result
                                  : operation.CopyItem(item, parent.Get(), nullptr, nullptr);
        });
    NotifyParentsChanged(sources);
    return report;
}

OperationReport ShellFileOperationGateway::RenameItems(
    const std::wstring& folder, const std::vector<domain::RenameStep>& steps) {
    BatchOptions options;
    options.stopAfterFirstFailure = true;

    OperationReport report;
    const auto sourceOf = [&](size_t index) { return domain::JoinPath(folder, steps[index].from); };
    const auto skipRange = [&](size_t from, size_t to) {
        for (size_t index = from; index < to; ++index) {
            report.items.push_back({sourceOf(index), {}, ItemStatus::NotAttempted, {}});
        }
    };
    const auto runRange = [&](size_t from, size_t to) {
        std::vector<std::wstring> sources;
        for (size_t index = from; index < to; ++index) {
            sources.push_back(sourceOf(index));
        }
        const OperationReport segment = RunBatch(
            ui_, sources, options, [&](IFileOperation& operation, IShellItem* item, size_t index) {
                return operation.RenameItem(item, steps[from + index].to.c_str(), nullptr);
            });
        report.items.insert(report.items.end(), segment.items.begin(), segment.items.end());
        return !HasProblem(segment);
    };

    for (size_t begin = 0; begin < steps.size();) {
        const size_t end = SegmentEnd(steps, begin);
        const size_t blocked = FirstBlockedStep(folder, steps, begin, end);
        const bool ranCleanly = blocked == begin || runRange(begin, blocked);
        if (!ranCleanly || blocked < end) {
            if (ranCleanly) {
                report.items.push_back({sourceOf(blocked), {}, ItemStatus::Failed,
                                        L"An item named " + steps[blocked].to + L" already exists."});
                skipRange(blocked + 1, steps.size());
            } else {
                skipRange(blocked, steps.size());
            }
            break;
        }
        begin = end;
    }
    NotifyFolderChanged(folder);
    return report;
}

}  // namespace et::infra
