#include <CppUnitTest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "Domain/RenamePlan.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

namespace {

using Names = std::vector<std::wstring>;

// Reverses the display order; the only way to make numbering targets form a cycle.
class ReversedCollation final : public domain::INameCollation {
public:
    bool Equals(std::wstring_view left, std::wstring_view right) const override {
        return simple_.Equals(left, right);
    }
    bool NaturalLess(std::wstring_view left, std::wstring_view right) const override {
        return simple_.NaturalLess(right, left);
    }

private:
    domain::SimpleNameCollation simple_;
};

domain::RenamePattern NameMask(const wchar_t* mask) {
    domain::RenamePattern pattern;
    pattern.nameMask = mask;
    return pattern;
}

// Plans a batch in a folder that contains exactly the selected files.
domain::Result<domain::RenamePlan> Plan(const Names& selected, const domain::RenamePattern& pattern,
                                        const domain::INameCollation& collation) {
    return domain::BuildRenamePlan(selected, selected, L"Trip", pattern, collation);
}

domain::Result<domain::RenamePlan> Plan(const Names& selected,
                                        const domain::RenamePattern& pattern = {}) {
    return Plan(selected, pattern, domain::SimpleNameCollation());
}

Names Renamed(const domain::RenamePlan& plan) {
    Names names;
    for (const auto& preview : plan.previews) {
        names.push_back(preview.renamed);
    }
    return names;
}

bool Contains(const Names& names, const std::wstring& name) {
    return std::find(names.begin(), names.end(), name) != names.end();
}

// Runs the steps against a folder listing, failing if any step would overwrite something.
Names Apply(const domain::RenamePlan& plan, Names folder) {
    for (const auto& step : plan.steps) {
        Assert::IsTrue(Contains(folder, step.from), (L"missing source " + step.from).c_str());
        Assert::IsFalse(Contains(folder, step.to), (L"occupied target " + step.to).c_str());
        *std::find(folder.begin(), folder.end(), step.from) = step.to;
    }
    std::sort(folder.begin(), folder.end());
    return folder;
}

}  // namespace

TEST_CLASS(RenamePlanTests) {
public:
    TEST_METHOD(DefaultAppendsUnderscoreAndIndexInNameOrder) {
        const auto plan = Plan({L"bia.png", L"anh.jpg"});
        Assert::IsTrue(plan.ok());
        Assert::IsTrue(Renamed(plan.value()) == Names{L"anh_01.jpg", L"bia_02.png"});
        Assert::AreEqual(std::wstring(L"anh.jpg"), plan.value().previews[0].original);
    }

    TEST_METHOD(FixedNameNumbersInNaturalOrder) {
        const auto plan = Plan({L"IMG_12.jpg", L"IMG_7.jpg"}, NameMask(L"DaLat_[C]"));
        Assert::IsTrue(plan.ok());
        Assert::AreEqual(std::wstring(L"IMG_7.jpg"), plan.value().previews[0].original);
        Assert::IsTrue(Renamed(plan.value()) == Names{L"DaLat_01.jpg", L"DaLat_02.jpg"});
    }

    TEST_METHOD(KeepsExtensionRules) {
        const auto plan = Plan({L".gitignore", L"archive.tar.gz", L"Makefile"});
        Assert::IsTrue(plan.ok());
        const Names renamed = Renamed(plan.value());
        Assert::IsTrue(Contains(renamed, L".gitignore_01"));
        Assert::IsTrue(Contains(renamed, L"archive.tar_02.gz"));
        Assert::IsTrue(Contains(renamed, L"Makefile_03"));
    }

    TEST_METHOD(MaskCanUseCounterFirstRangesAndFolderName) {
        const auto plan = Plan({L"IMG_7.jpg", L"IMG_12.jpg"}, NameMask(L"[C] [P] [N1-3]"));
        Assert::IsTrue(plan.ok());
        Assert::IsTrue(Renamed(plan.value()) == Names{L"01 Trip IMG.jpg", L"02 Trip IMG.jpg"});
    }

    TEST_METHOD(ExtensionMaskReplacesOrRemovesExtension) {
        domain::RenamePattern pattern;
        pattern.extensionMask = L"jpeg";
        Assert::IsTrue(Renamed(Plan({L"a.jpg"}, pattern).value()) == Names{L"a_01.jpeg"});

        pattern.extensionMask = L"";
        Assert::IsTrue(Renamed(Plan({L"a.jpg"}, pattern).value()) == Names{L"a_01"});
    }

    TEST_METHOD(CounterHonoursStartStepAndDigits) {
        domain::RenamePattern pattern = NameMask(L"x[C]");
        pattern.counterStart = 10;
        pattern.counterStep = 5;
        pattern.counterDigits = 4;
        const auto plan = Plan({L"a.txt", L"b.txt", L"c.txt"}, pattern);
        Assert::IsTrue(plan.ok());
        Assert::IsTrue(Renamed(plan.value()) == Names{L"x0010.txt", L"x0015.txt", L"x0020.txt"});
    }

    TEST_METHOD(CounterWidthGrowsWithItemCount) {
        const auto firstAndLast = [&](size_t count) {
            Names selected;
            for (size_t index = 0; index < count; ++index) {
                selected.push_back(L"f" + std::to_wstring(index) + L".txt");
            }
            const auto plan = Plan(selected, NameMask(L"x_[C]"));
            Assert::IsTrue(plan.ok());
            return std::pair{plan.value().previews.front().renamed,
                             plan.value().previews.back().renamed};
        };
        using Pair = std::pair<std::wstring, std::wstring>;
        Assert::IsTrue(firstAndLast(9) == Pair{L"x_01.txt", L"x_09.txt"});
        Assert::IsTrue(firstAndLast(99).second == L"x_99.txt");
        Assert::IsTrue(firstAndLast(100) == Pair{L"x_001.txt", L"x_100.txt"});
        Assert::IsTrue(firstAndLast(1000).first == L"x_0001.txt");
    }

    TEST_METHOD(SearchAndReplaceAppliesToWholeNewName) {
        domain::RenamePattern pattern = NameMask(L"[N]");
        pattern.searchFor = L"IMG";
        pattern.replaceWith = L"Photo";
        Assert::IsTrue(Renamed(Plan({L"IMG_IMG_1.jpg"}, pattern).value()) ==
                       Names{L"Photo_Photo_1.jpg"});

        pattern.searchFor = L".jpg";
        pattern.replaceWith = L".jpeg";
        Assert::IsTrue(Renamed(Plan({L"a.jpg"}, pattern).value()) == Names{L"a.jpeg"});

        pattern.searchFor = L"a";
        pattern.replaceWith = L"aa";
        Assert::IsTrue(Renamed(Plan({L"aa.txt"}, pattern).value()) == Names{L"aaaa.txt"});
    }

    TEST_METHOD(MaskWithoutCounterCollidesAndIsRejected) {
        const auto plan = Plan({L"a.txt", L"b.txt"}, NameMask(L"same"));
        Assert::IsFalse(plan.ok());
        Assert::IsTrue(plan.error().code == domain::ErrorCode::NameCollision);
    }

    TEST_METHOD(RejectsCollisionWithItemOutsideSelection) {
        const domain::SimpleNameCollation collation;
        const auto plan =
            domain::BuildRenamePlan({L"a.txt"}, {L"a.txt", L"A_01.TXT"}, L"Trip", {}, collation);
        Assert::IsFalse(plan.ok());
        Assert::IsTrue(plan.error().code == domain::ErrorCode::NameCollision);
    }

    TEST_METHOD(RejectsInvalidResultingNameAndInvalidMask) {
        const auto invalidName = Plan({L"a.txt"}, NameMask(L"[N]:[C]"));
        Assert::IsTrue(invalidName.error().code == domain::ErrorCode::InvalidName);

        const auto emptyName = Plan({L"a.txt"}, NameMask(L""));
        Assert::IsFalse(emptyName.ok());

        const auto badMask = Plan({L"a.txt"}, NameMask(L"[Q]"));
        Assert::IsTrue(badMask.error().code == domain::ErrorCode::InvalidArgument);

        domain::RenamePattern badExtension;
        badExtension.extensionMask = L"[E";
        Assert::IsTrue(Plan({L"a.txt"}, badExtension).error().code ==
                       domain::ErrorCode::InvalidArgument);
    }

    TEST_METHOD(RejectsEmptySelectionAndAbsurdCounter) {
        Assert::IsFalse(Plan({}).ok());
        domain::RenamePattern pattern;
        pattern.counterStart = 1000000;
        Assert::IsFalse(Plan({L"a.txt"}, pattern).ok());
        pattern = {};
        pattern.counterDigits = 10;
        Assert::IsFalse(Plan({L"a.txt"}, pattern).ok());
    }

    TEST_METHOD(UnchangedNamesProduceNoStep) {
        const auto plan = Plan({L"x_01.txt", L"x_02.txt"}, NameMask(L"x_[C]"));
        Assert::IsTrue(plan.ok());
        Assert::IsTrue(plan.value().steps.empty());
        Assert::AreEqual(size_t{2}, plan.value().previews.size());
    }

    // Re-running on an already numbered batch plus a new file: every target is the current
    // name of the next file, so the steps must run back to front.
    TEST_METHOD(OrdersDependentRenamesSoTargetsAreFree) {
        const Names selected{L"x_01.txt", L"x_02.txt", L"a.txt"};
        const auto plan = Plan(selected, NameMask(L"x_[C]"));
        Assert::IsTrue(plan.ok());
        Assert::AreEqual(size_t{3}, plan.value().steps.size());
        Assert::IsTrue(Apply(plan.value(), selected) == Names{L"x_01.txt", L"x_02.txt", L"x_03.txt"});
    }

    TEST_METHOD(BreaksCycleThroughTemporaryName) {
        domain::RenamePattern pattern = NameMask(L"p_[C]");
        pattern.counterDigits = 1;
        const Names selected{L"p_1.txt", L"p_2.txt"};
        const auto plan = Plan(selected, pattern, ReversedCollation());
        Assert::IsTrue(plan.ok());
        Assert::AreEqual(size_t{3}, plan.value().steps.size());
        Assert::IsTrue(Apply(plan.value(), selected) == Names{L"p_1.txt", L"p_2.txt"});
        Assert::AreEqual(std::wstring(L"p_2.txt"), plan.value().previews[0].original);
        Assert::AreEqual(std::wstring(L"p_1.txt"), plan.value().previews[0].renamed);
    }

    TEST_METHOD(TemporaryNameAvoidsExistingItems) {
        const ReversedCollation collation;
        domain::RenamePattern pattern = NameMask(L"p_[C]");
        pattern.counterDigits = 1;
        const Names folder{L"p_1.txt", L"p_2.txt", L"~exmate-1.tmp"};
        const auto plan =
            domain::BuildRenamePlan({L"p_1.txt", L"p_2.txt"}, folder, L"Trip", pattern, collation);
        Assert::IsTrue(plan.ok());
        Apply(plan.value(), folder);
    }
};

}  // namespace et::tests
