#include <CppUnitTest.h>

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "Application/GroupIntoNewFolderUseCase.h"
#include "Infrastructure/ShellFileOperationGateway.h"
#include "Infrastructure/Win32FileSystemProbe.h"
#include "Infrastructure/WindowsNameCollation.h"
#include "TempFixture.h"
#include "TestSupport.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

namespace {

namespace fs = std::filesystem;
using domain::ItemStatus;
using domain::OperationReport;
using infra::OperationUi;
using infra::ShellFileOperationGateway;

const wchar_t* const kVietnameseName = L"ảnh đẹp.jpg";

class FixedFolderNamePrompt final : public app::IUserPrompt {
public:
    explicit FixedFolderNamePrompt(std::wstring name) : name_(std::move(name)) {}
    std::optional<std::wstring> AskFolderName(const std::wstring&, const app::NameValidator&) override {
        return name_;
    }
    std::optional<domain::RenamePattern> AskRenamePattern(const domain::RenamePattern&,
                                                          const app::RenamePreviewer&) override {
        return std::nullopt;
    }

private:
    std::wstring name_;
};

}  // namespace

TEST_CLASS(ShellGatewayTests) {
public:
    TEST_METHOD(DuplicateCopiesFileNextToOriginalWithoutTouchingIt) {
        const TempFixture fixture;
        const fs::path original = fixture.root() / kVietnameseName;
        WriteText(original, "payload");

        const OperationReport report = RunInSta([&] {
            return ShellFileOperationGateway(OperationUi::Silent).DuplicateItems({original.wstring()});
        });

        Assert::IsTrue(report.AllSucceeded());
        const fs::path copy = report.items[0].destination;
        Assert::IsTrue(copy != original);
        Assert::IsTrue(copy.parent_path() == fixture.root());
        Assert::IsTrue(copy.extension() == L".jpg");
        Assert::AreEqual(std::string("payload"), ReadText(copy));
        Assert::AreEqual(std::string("payload"), ReadText(original));
    }

    TEST_METHOD(DuplicateTwiceCreatesTwoDistinctCopies) {
        const TempFixture fixture;
        const fs::path original = fixture.root() / L"a.txt";
        WriteText(original, "1");

        const auto destinations = RunInSta([&] {
            ShellFileOperationGateway gateway(OperationUi::Silent);
            std::vector<std::wstring> result;
            result.push_back(gateway.DuplicateItems({original.wstring()}).items[0].destination);
            result.push_back(gateway.DuplicateItems({original.wstring()}).items[0].destination);
            return result;
        });

        Assert::IsFalse(destinations[0].empty());
        Assert::IsTrue(destinations[0] != destinations[1]);
        Assert::IsTrue(fs::exists(destinations[0]) && fs::exists(destinations[1]));
    }

    TEST_METHOD(DuplicateCopiesWholeFolderTreeWithoutMerging) {
        const TempFixture fixture;
        const fs::path folder = fixture.root() / L"Assets";
        WriteText(folder / L"logo.png", "logo");
        WriteText(folder / L"sub" / L"deep.txt", "deep");
        WriteText(fixture.root() / L"a.txt", "a");

        const OperationReport report = RunInSta([&] {
            return ShellFileOperationGateway(OperationUi::Silent)
                .DuplicateItems({folder.wstring(), (fixture.root() / L"a.txt").wstring()});
        });

        Assert::IsTrue(report.AllSucceeded());
        const fs::path copy = report.items[0].destination;
        Assert::IsTrue(copy != folder);
        Assert::AreEqual(std::string("logo"), ReadText(copy / L"logo.png"));
        Assert::AreEqual(std::string("deep"), ReadText(copy / L"sub" / L"deep.txt"));
        Assert::AreEqual(std::string("deep"), ReadText(folder / L"sub" / L"deep.txt"));
        Assert::IsTrue(fs::exists(report.items[1].destination));
    }

    TEST_METHOD(CreateNewFolderRefusesExistingName) {
        const TempFixture fixture;
        const std::wstring path = (fixture.root() / L"Project A").wstring();
        ShellFileOperationGateway gateway(OperationUi::Silent);

        Assert::IsTrue(gateway.CreateNewFolder(path).ok());
        const auto second = gateway.CreateNewFolder(path);
        Assert::IsFalse(second.ok());
        Assert::IsTrue(second.error().code == domain::ErrorCode::NameCollision);
    }

    TEST_METHOD(RemoveFolderIfEmptyNeverDeletesContent) {
        const TempFixture fixture;
        const fs::path folder = fixture.root() / L"Keep";
        WriteText(folder / L"data.txt", "x");
        ShellFileOperationGateway gateway(OperationUi::Silent);

        Assert::IsFalse(gateway.RemoveFolderIfEmpty(folder.wstring()).ok());
        Assert::IsTrue(fs::exists(folder / L"data.txt"));

        fs::remove(folder / L"data.txt");
        Assert::IsTrue(gateway.RemoveFolderIfEmpty(folder.wstring()).ok());
        Assert::IsFalse(fs::exists(folder));
    }

    TEST_METHOD(MoveKeepsSubtreeAndReportsDestinations) {
        const TempFixture fixture;
        WriteText(fixture.root() / L"a.txt", "a");
        WriteText(fixture.root() / L"Assets" / L"logo.png", "logo");
        const fs::path destination = fixture.root() / L"Project A";
        fs::create_directory(destination);

        const OperationReport report = RunInSta([&] {
            return ShellFileOperationGateway(OperationUi::Silent)
                .MoveItemsInto({(fixture.root() / L"a.txt").wstring(),
                                (fixture.root() / L"Assets").wstring()},
                               destination.wstring());
        });

        Assert::IsTrue(report.AllSucceeded());
        Assert::AreEqual(std::string("a"), ReadText(destination / L"a.txt"));
        Assert::AreEqual(std::string("logo"), ReadText(destination / L"Assets" / L"logo.png"));
        Assert::IsFalse(fs::exists(fixture.root() / L"a.txt"));
        Assert::IsFalse(fs::exists(fixture.root() / L"Assets"));
        Assert::IsTrue(fs::path(report.items[0].destination) == destination / L"a.txt");
    }

    TEST_METHOD(MoveReportsMissingSourceAndStillMovesTheRest) {
        const TempFixture fixture;
        WriteText(fixture.root() / L"a.txt", "a");
        const fs::path destination = fixture.root() / L"Dest";
        fs::create_directory(destination);

        const OperationReport report = RunInSta([&] {
            return ShellFileOperationGateway(OperationUi::Silent)
                .MoveItemsInto({(fixture.root() / L"gone.txt").wstring(),
                                (fixture.root() / L"a.txt").wstring()},
                               destination.wstring());
        });

        Assert::IsTrue(report.items[0].status == ItemStatus::Failed);
        Assert::IsFalse(report.items[0].detail.empty());
        Assert::IsTrue(report.items[1].status == ItemStatus::Succeeded);
        Assert::IsTrue(fs::exists(destination / L"a.txt"));
    }

    TEST_METHOD(RenameRunsDependentStepsInOrder) {
        const TempFixture fixture;
        WriteText(fixture.root() / L"x_01.txt", "one");
        WriteText(fixture.root() / L"x_02.txt", "two");
        WriteText(fixture.root() / L"a.txt", "new");

        const OperationReport report = RunInSta([&] {
            return ShellFileOperationGateway(OperationUi::Silent)
                .RenameItems(fixture.root().wstring(), {{L"x_02.txt", L"x_03.txt"},
                                                        {L"x_01.txt", L"x_02.txt"},
                                                        {L"a.txt", L"x_01.txt"}});
        });

        Assert::IsTrue(report.AllSucceeded());
        Assert::AreEqual(std::string("new"), ReadText(fixture.root() / L"x_01.txt"));
        Assert::AreEqual(std::string("one"), ReadText(fixture.root() / L"x_02.txt"));
        Assert::AreEqual(std::string("two"), ReadText(fixture.root() / L"x_03.txt"));
        Assert::IsFalse(fs::exists(fixture.root() / L"a.txt"));
    }

    TEST_METHOD(RenameSwapsThroughTemporaryName) {
        const TempFixture fixture;
        WriteText(fixture.root() / L"p_1.txt", "first");
        WriteText(fixture.root() / L"p_2.txt", "second");

        const OperationReport report = RunInSta([&] {
            return ShellFileOperationGateway(OperationUi::Silent)
                .RenameItems(fixture.root().wstring(), {{L"p_1.txt", L"~tmp.tmp"},
                                                        {L"p_2.txt", L"p_1.txt"},
                                                        {L"~tmp.tmp", L"p_2.txt"}});
        });

        Assert::IsTrue(report.AllSucceeded());
        Assert::AreEqual(size_t{3}, report.items.size());
        Assert::AreEqual(std::string("second"), ReadText(fixture.root() / L"p_1.txt"));
        Assert::AreEqual(std::string("first"), ReadText(fixture.root() / L"p_2.txt"));
        Assert::IsFalse(fs::exists(fixture.root() / L"~tmp.tmp"));
    }

    TEST_METHOD(RenameStopsAtFirstFailure) {
        const TempFixture fixture;
        const fs::path locked = fixture.root() / L"a.txt";
        WriteText(locked, "a");
        WriteText(fixture.root() / L"b.txt", "b");
        const HANDLE lock = CreateFileW(locked.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, nullptr);
        Assert::IsTrue(lock != INVALID_HANDLE_VALUE);

        const OperationReport report = RunInSta([&] {
            return ShellFileOperationGateway(OperationUi::Silent)
                .RenameItems(fixture.root().wstring(),
                             {{L"a.txt", L"a_01.txt"}, {L"b.txt", L"b_02.txt"}});
        });
        CloseHandle(lock);

        Assert::IsTrue(report.items[0].status == ItemStatus::Failed);
        Assert::IsFalse(report.items[0].detail.empty());
        Assert::IsTrue(report.items[1].status == ItemStatus::NotAttempted);
        Assert::IsTrue(fs::exists(locked));
        Assert::IsTrue(fs::exists(fixture.root() / L"b.txt"));
        Assert::IsFalse(fs::exists(fixture.root() / L"b_02.txt"));
    }

    // The planner rules this out, but another process can create the target in between.
    // The gateway must refuse before the Shell gets a chance to ask "replace or skip?".
    TEST_METHOD(RenameNeverOverwritesExistingTarget) {
        const TempFixture fixture;
        WriteText(fixture.root() / L"a.txt", "source");
        WriteText(fixture.root() / L"b.txt", "bystander");
        WriteText(fixture.root() / L"c.txt", "later");

        const OperationReport report = RunInSta([&] {
            return ShellFileOperationGateway(OperationUi::Silent)
                .RenameItems(fixture.root().wstring(),
                             {{L"a.txt", L"b.txt"}, {L"c.txt", L"d.txt"}});
        });

        Assert::IsTrue(report.items[0].status == ItemStatus::Failed);
        Assert::IsTrue(report.items[1].status == ItemStatus::NotAttempted);
        Assert::AreEqual(std::string("bystander"), ReadText(fixture.root() / L"b.txt"));
        Assert::AreEqual(std::string("source"), ReadText(fixture.root() / L"a.txt"));
        Assert::IsTrue(fs::exists(fixture.root() / L"c.txt"));
    }

    TEST_METHOD(ProbeDescribesItemsAndListsFolder) {
        const TempFixture fixture;
        WriteText(fixture.root() / L"a.txt", "a");
        fs::create_directory(fixture.root() / L"Sub");
        const infra::Win32FileSystemProbe probe;

        Assert::IsTrue(probe.Inspect((fixture.root() / L"a.txt").wstring()).exists);
        Assert::IsFalse(probe.Inspect((fixture.root() / L"a.txt").wstring()).isDirectory);
        Assert::IsTrue(probe.Inspect((fixture.root() / L"Sub").wstring()).isDirectory);
        Assert::IsFalse(probe.Inspect((fixture.root() / L"gone").wstring()).exists);

        auto names = probe.ListNames(fixture.root().wstring());
        Assert::IsTrue(names.ok());
        std::vector<std::wstring> sorted = names.value();
        std::sort(sorted.begin(), sorted.end());
        Assert::IsTrue(sorted == std::vector<std::wstring>{L".exmate-fixture", L"Sub", L"a.txt"});
        Assert::IsFalse(probe.ListNames((fixture.root() / L"gone").wstring()).ok());
    }

    TEST_METHOD(ProbeFlagsJunctionAsReparsePoint) {
        const TempFixture fixture;
        const fs::path target = fixture.root() / L"Target";
        const fs::path link = fixture.root() / L"Link";
        fs::create_directory(target);
        std::error_code error;
        fs::create_directory_symlink(target, link, error);
        if (error) {
            // Creating symlinks needs Developer Mode or elevation; nothing to assert without one.
            return;
        }
        Assert::IsTrue(infra::Win32FileSystemProbe().Inspect(link.wstring()).isReparsePoint);
        fs::remove(link);
    }

    TEST_METHOD(CollationMatchesWindowsRules) {
        const infra::WindowsNameCollation collation;
        Assert::IsTrue(collation.Equals(L"ẢNH.JPG", L"ảnh.jpg"));
        Assert::IsFalse(collation.Equals(L"a.txt", L"b.txt"));
        Assert::IsTrue(collation.NaturalLess(L"IMG_7.jpg", L"IMG_12.jpg"));
        Assert::IsFalse(collation.NaturalLess(L"IMG_12.jpg", L"IMG_7.jpg"));
        Assert::IsTrue(collation.NaturalLess(L"a01", L"a1") != collation.NaturalLess(L"a1", L"a01"));
    }

    TEST_METHOD(GroupUseCaseWorksEndToEndOnRealFilesystem) {
        const TempFixture fixture;
        WriteText(fixture.root() / L"a.txt", "a");
        WriteText(fixture.root() / kVietnameseName, "img");
        WriteText(fixture.root() / L"Assets" / L"logo.png", "logo");

        const auto report = RunInSta([&] {
            const infra::Win32FileSystemProbe probe;
            const infra::WindowsNameCollation collation;
            ShellFileOperationGateway gateway(OperationUi::Silent);
            FixedFolderNamePrompt prompt(L"Dự án A");
            app::GroupIntoNewFolderUseCase useCase(probe, gateway, prompt, collation);
            auto result = useCase.Execute({(fixture.root() / L"a.txt").wstring(),
                                           (fixture.root() / kVietnameseName).wstring(),
                                           (fixture.root() / L"Assets").wstring()});
            return result.ok() ? std::move(result).value() : OperationReport{};
        });

        const fs::path group = fixture.root() / L"Dự án A";
        Assert::AreEqual(size_t{3}, report.items.size());
        Assert::IsTrue(report.AllSucceeded());
        Assert::AreEqual(std::string("a"), ReadText(group / L"a.txt"));
        Assert::AreEqual(std::string("img"), ReadText(group / kVietnameseName));
        Assert::AreEqual(std::string("logo"), ReadText(group / L"Assets" / L"logo.png"));
        Assert::IsFalse(fs::exists(fixture.root() / L"a.txt"));
    }
};

}  // namespace et::tests
