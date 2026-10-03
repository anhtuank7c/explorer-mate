#include "Domain/RenamePlan.h"

#include <algorithm>
#include <utility>

#include "Domain/ItemName.h"
#include "Domain/RenameMask.h"

namespace et::domain {

namespace {

constexpr unsigned kMaxCounterValue = 999999;
constexpr unsigned kMaxCounterDigits = 9;

bool ContainsName(const std::vector<std::wstring>& names, std::wstring_view name,
                  const INameCollation& collation) {
    return std::any_of(names.begin(), names.end(), [&](const std::wstring& candidate) {
        return collation.Equals(candidate, name);
    });
}

std::wstring PadIndex(size_t index, size_t width) {
    std::wstring digits = std::to_wstring(index);
    if (digits.size() < width) {
        digits.insert(0, width - digits.size(), L'0');
    }
    return digits;
}

std::optional<Error> ValidatePattern(const RenamePattern& pattern) {
    if (pattern.counterStart > kMaxCounterValue || pattern.counterStep > kMaxCounterValue) {
        return Error(ErrorCode::InvalidArgument,
                     L"The counter start and step cannot exceed 999999.");
    }
    if (pattern.counterDigits > kMaxCounterDigits) {
        return Error(ErrorCode::InvalidArgument, L"The counter cannot have more than 9 digits.");
    }
    if (const auto invalid = ValidateMask(pattern.nameMask)) {
        return invalid;
    }
    return ValidateMask(pattern.extensionMask);
}

void ReplaceAll(std::wstring& text, const std::wstring& searchFor, const std::wstring& replaceWith) {
    if (searchFor.empty()) {
        return;
    }
    for (size_t position = text.find(searchFor); position != std::wstring::npos;
         position = text.find(searchFor, position + replaceWith.size())) {
        text.replace(position, searchFor.size(), replaceWith);
    }
}

// Expands the masks for every file, in numbering order. The pattern must already be valid.
Result<std::vector<RenamePreview>> ExpandNames(const std::vector<std::wstring>& sortedNames,
                                               std::wstring_view parentName,
                                               const RenamePattern& pattern) {
    const size_t lastCounter =
        pattern.counterStart + static_cast<size_t>(pattern.counterStep) * (sortedNames.size() - 1);
    const size_t width =
        std::max<size_t>(pattern.counterDigits, std::to_wstring(lastCounter).size());

    std::vector<RenamePreview> previews;
    previews.reserve(sortedNames.size());
    for (size_t position = 0; position < sortedNames.size(); ++position) {
        const NameParts parts = SplitStemAndExtension(sortedNames[position]);
        const std::wstring counter =
            PadIndex(pattern.counterStart + static_cast<size_t>(pattern.counterStep) * position, width);
        const std::wstring_view extension =
            parts.extension.empty() ? std::wstring_view() : std::wstring_view(parts.extension).substr(1);
        const MaskContext context{parts.stem, extension, counter, parentName};

        const auto name = ExpandMask(pattern.nameMask, context);
        if (!name.ok()) {
            return name.error();
        }
        // "" + ".txt" would be a legal dotfile name, but never what the user meant.
        if (name.value().empty()) {
            return Error(ErrorCode::InvalidName,
                         L"The file name mask produces an empty name for " + sortedNames[position]);
        }
        const auto newExtension = ExpandMask(pattern.extensionMask, context);
        if (!newExtension.ok()) {
            return newExtension.error();
        }
        std::wstring renamed = name.value();
        if (!newExtension.value().empty()) {
            renamed.append(L".").append(newExtension.value());
        }
        ReplaceAll(renamed, pattern.searchFor, pattern.replaceWith);
        previews.push_back({sortedNames[position], std::move(renamed)});
    }
    return previews;
}

std::optional<Error> FindProblem(const std::vector<RenamePreview>& previews,
                                 const std::vector<std::wstring>& selectedNames,
                                 const std::vector<std::wstring>& siblingNames,
                                 const INameCollation& collation) {
    for (size_t index = 0; index < previews.size(); ++index) {
        const std::wstring& target = previews[index].renamed;
        if (const auto invalid = ValidateItemName(target)) {
            return Error(ErrorCode::InvalidName, target + L": " + invalid->message);
        }
        for (size_t earlier = 0; earlier < index; ++earlier) {
            if (collation.Equals(previews[earlier].renamed, target)) {
                return Error(ErrorCode::NameCollision,
                             L"Two files would both be renamed to " + target);
            }
        }
        const bool takenByOutsider = ContainsName(siblingNames, target, collation) &&
                                     !ContainsName(selectedNames, target, collation);
        if (takenByOutsider) {
            return Error(ErrorCode::NameCollision,
                         L"An item named " + target + L" already exists in this folder.");
        }
    }
    return std::nullopt;
}

std::wstring MakeTemporaryName(const std::vector<std::wstring>& siblingNames,
                               const std::vector<RenamePreview>& previews,
                               const std::vector<RenameStep>& steps,
                               const INameCollation& collation) {
    for (size_t counter = 1;; ++counter) {
        const std::wstring candidate = L"~exmate-" + std::to_wstring(counter) + L".tmp";
        const bool usedByTarget =
            std::any_of(previews.begin(), previews.end(), [&](const RenamePreview& preview) {
                return collation.Equals(preview.renamed, candidate);
            });
        const bool usedByStep = std::any_of(steps.begin(), steps.end(), [&](const RenameStep& step) {
            return collation.Equals(step.to, candidate);
        });
        if (!usedByTarget && !usedByStep && !ContainsName(siblingNames, candidate, collation)) {
            return candidate;
        }
    }
}

// Orders the renames so that no step targets a name another pending step still occupies.
// When every pending step is blocked the targets form a cycle; parking one source under a
// temporary name breaks it.
std::vector<RenameStep> OrderSteps(const std::vector<RenamePreview>& previews,
                                   const std::vector<std::wstring>& siblingNames,
                                   const INameCollation& collation) {
    std::vector<RenameStep> pending;
    for (const RenamePreview& preview : previews) {
        if (preview.original != preview.renamed) {
            pending.push_back({preview.original, preview.renamed});
        }
    }

    std::vector<RenameStep> ordered;
    while (!pending.empty()) {
        const auto unblocked =
            std::find_if(pending.begin(), pending.end(), [&](const RenameStep& step) {
                return std::none_of(pending.begin(), pending.end(), [&](const RenameStep& other) {
                    return &other != &step && collation.Equals(other.from, step.to);
                });
            });
        if (unblocked != pending.end()) {
            ordered.push_back(*unblocked);
            pending.erase(unblocked);
            continue;
        }
        RenameStep& parked = pending.front();
        std::wstring temporary = MakeTemporaryName(siblingNames, previews, ordered, collation);
        ordered.push_back({parked.from, temporary});
        parked.from = std::move(temporary);
    }
    return ordered;
}

}  // namespace

Result<RenamePlan> BuildRenamePlan(std::vector<std::wstring> selectedNames,
                                   const std::vector<std::wstring>& siblingNames,
                                   std::wstring_view parentName,
                                   const RenamePattern& pattern,
                                   const INameCollation& collation) {
    if (selectedNames.empty()) {
        return Error(ErrorCode::EmptySelection, L"Nothing is selected.");
    }
    if (const auto invalid = ValidatePattern(pattern)) {
        return *invalid;
    }

    std::sort(selectedNames.begin(), selectedNames.end(),
              [&](const std::wstring& left, const std::wstring& right) {
                  return collation.NaturalLess(left, right);
              });

    auto previews = ExpandNames(selectedNames, parentName, pattern);
    if (!previews.ok()) {
        return previews.error();
    }
    RenamePlan plan;
    plan.previews = std::move(previews).value();
    if (const auto problem = FindProblem(plan.previews, selectedNames, siblingNames, collation)) {
        return *problem;
    }
    plan.steps = OrderSteps(plan.previews, siblingNames, collation);
    return plan;
}

}  // namespace et::domain
