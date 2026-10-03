#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "Application/Ports/IFileOperationGateway.h"
#include "Application/Ports/IFileSystemProbe.h"
#include "Application/Ports/IUserPrompt.h"
#include "Domain/ItemName.h"
#include "Domain/PathText.h"

namespace et::tests {

inline std::wstring FoldCase(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t character) {
        return (character >= L'A' && character <= L'Z')
                   ? static_cast<wchar_t>(character + (L'a' - L'A'))
                   : character;
    });
    return text;
}

// In-memory, case-insensitive filesystem implementing both filesystem ports.
class FakeFileSystem final : public app::IFileSystemProbe, public app::IFileOperationGateway {
public:
    // --- Test setup and inspection -------------------------------------------------------
    void AddFile(const std::wstring& path) { entries_[FoldCase(path)] = {path, false, {}}; }
    void AddFolder(const std::wstring& path) { entries_[FoldCase(path)] = {path, true, {}}; }
    void MarkReparsePoint(const std::wstring& path) { entries_.at(FoldCase(path)).reparse = true; }
    bool Exists(const std::wstring& path) const { return entries_.count(FoldCase(path)) != 0; }
    size_t EntryCount() const { return entries_.size(); }

    std::set<std::wstring> failingSources;         // Folded paths whose move/copy/rename fails.
    std::function<void()> beforeCreateNewFolder;   // Simulates another process racing us.
    int mutationCount = 0;

    // --- IFileSystemProbe ----------------------------------------------------------------
    app::ItemInfo Inspect(const std::wstring& path) const override {
        const auto found = entries_.find(FoldCase(path));
        if (found == entries_.end()) {
            return {};
        }
        app::ItemInfo info;
        info.exists = true;
        info.isDirectory = found->second.isFolder;
        info.isReparsePoint = found->second.reparse;
        return info;
    }

    domain::Result<std::vector<std::wstring>> ListNames(const std::wstring& folder) const override {
        std::vector<std::wstring> names;
        for (const auto& [key, entry] : entries_) {
            if (FoldCase(std::wstring(domain::ParentOf(entry.path))) == FoldCase(folder)) {
                names.emplace_back(domain::NameOf(entry.path));
            }
        }
        return names;
    }

    // --- IFileOperationGateway -----------------------------------------------------------
    domain::Status CreateNewFolder(const std::wstring& path) override {
        if (beforeCreateNewFolder) {
            beforeCreateNewFolder();
        }
        if (Exists(path)) {
            return domain::Error(domain::ErrorCode::NameCollision, L"exists: " + path);
        }
        AddFolder(path);
        ++mutationCount;
        return domain::Unit{};
    }

    domain::Status RemoveFolderIfEmpty(const std::wstring& path) override {
        if (!ListNames(path).value().empty()) {
            return domain::Error(domain::ErrorCode::OperationFailed, L"not empty: " + path);
        }
        entries_.erase(FoldCase(path));
        ++mutationCount;
        return domain::Unit{};
    }

    domain::OperationReport MoveItemsInto(const std::vector<std::wstring>& sources,
                                          const std::wstring& destinationFolder) override {
        domain::OperationReport report;
        for (const std::wstring& source : sources) {
            const std::wstring target =
                domain::JoinPath(destinationFolder, domain::NameOf(source));
            report.items.push_back(Relocate(source, target, false));
        }
        return report;
    }

    domain::OperationReport DuplicateItems(const std::vector<std::wstring>& sources) override {
        domain::OperationReport report;
        for (const std::wstring& source : sources) {
            report.items.push_back(Relocate(source, FreeCopyPath(source), true));
        }
        return report;
    }

    domain::OperationReport RenameItems(const std::wstring& folder,
                                        const std::vector<domain::RenameStep>& steps) override {
        domain::OperationReport report;
        bool stopped = false;
        for (const domain::RenameStep& step : steps) {
            const std::wstring source = domain::JoinPath(folder, step.from);
            if (stopped) {
                report.items.push_back({source, {}, domain::ItemStatus::NotAttempted, {}});
                continue;
            }
            report.items.push_back(Relocate(source, domain::JoinPath(folder, step.to), false));
            stopped = report.items.back().status != domain::ItemStatus::Succeeded;
        }
        return report;
    }

private:
    struct Entry {
        std::wstring path;
        bool isFolder = false;
        bool reparse = false;
    };

    std::wstring FreeCopyPath(const std::wstring& source) const {
        const domain::NameParts parts = domain::SplitStemAndExtension(domain::NameOf(source));
        const std::wstring parent(domain::ParentOf(source));
        std::wstring candidate = domain::JoinPath(parent, parts.stem + L" - Copy" + parts.extension);
        for (unsigned counter = 2; Exists(candidate); ++counter) {
            candidate = domain::JoinPath(parent, parts.stem + L" - Copy (" +
                                                     std::to_wstring(counter) + L")" +
                                                     parts.extension);
        }
        return candidate;
    }

    // Moves or copies `source` and everything beneath it to `target`. Never overwrites.
    domain::ItemOutcome Relocate(const std::wstring& source, const std::wstring& target,
                                 bool keepSource) {
        if (failingSources.count(FoldCase(source)) != 0) {
            return {source, {}, domain::ItemStatus::Failed, L"injected failure"};
        }
        if (!Exists(source)) {
            return {source, {}, domain::ItemStatus::Failed, L"source missing"};
        }
        if (Exists(target)) {
            return {source, {}, domain::ItemStatus::Failed, L"target exists"};
        }
        const std::wstring sourceKey = FoldCase(source);
        std::vector<Entry> relocated;
        for (auto iterator = entries_.begin(); iterator != entries_.end();) {
            const bool isSelf = iterator->first == sourceKey;
            const bool isDescendant = iterator->first.rfind(sourceKey + L"\\", 0) == 0;
            if (!isSelf && !isDescendant) {
                ++iterator;
                continue;
            }
            Entry moved = iterator->second;
            moved.path = target + moved.path.substr(source.size());
            relocated.push_back(moved);
            iterator = keepSource ? std::next(iterator) : entries_.erase(iterator);
        }
        for (const Entry& entry : relocated) {
            entries_[FoldCase(entry.path)] = entry;
        }
        ++mutationCount;
        return {source, target, domain::ItemStatus::Succeeded, {}};
    }

    std::map<std::wstring, Entry> entries_;
};

// Answers prompts with pre-set values and records what the use case offered.
class ScriptedPrompt final : public app::IUserPrompt {
public:
    std::optional<std::wstring> folderNameAnswer;
    std::optional<domain::RenamePattern> patternAnswer;

    std::wstring offeredFolderName;
    std::optional<std::wstring> folderNameProblem;
    std::optional<domain::Result<std::vector<domain::RenamePreview>>> previewOfAnswer;
    int promptCount = 0;

    std::optional<std::wstring> AskFolderName(const std::wstring& suggestion,
                                              const app::NameValidator& validate) override {
        ++promptCount;
        offeredFolderName = suggestion;
        if (folderNameAnswer) {
            folderNameProblem = validate(*folderNameAnswer);
        }
        return folderNameAnswer;
    }

    std::optional<domain::RenamePattern> AskRenamePattern(
        const domain::RenamePattern&, const app::RenamePreviewer& preview) override {
        ++promptCount;
        if (patternAnswer) {
            previewOfAnswer = preview(*patternAnswer);
        }
        return patternAnswer;
    }
};

}  // namespace et::tests
