#include <CppUnitTest.h>

#include <string>
#include <vector>

#include "Application/BulkRenameUseCase.h"
#include "Application/DuplicateInPlaceUseCase.h"
#include "Application/GroupIntoNewFolderUseCase.h"
#include "Application/SelectionGuard.h"
#include "Domain/Selection.h"
#include "Fakes.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

namespace {

using Paths = std::vector<std::wstring>;
using domain::ErrorCode;
using domain::ItemStatus;

// D:\Work with a.txt, b.jpg and Assets\logo.png.
FakeFileSystem WorkFolder() {
    FakeFileSystem files;
    files.AddFolder(L"D:\\Work");
    files.AddFile(L"D:\\Work\\a.txt");
    files.AddFile(L"D:\\Work\\b.jpg");
    files.AddFolder(L"D:\\Work\\Assets");
    files.AddFile(L"D:\\Work\\Assets\\logo.png");
    return files;
}

const Paths kMixedSelection{L"D:\\Work\\a.txt", L"D:\\Work\\b.jpg", L"D:\\Work\\Assets"};

}  // namespace

TEST_CLASS(SelectionTests) {
public:
    TEST_METHOD(AcceptsItemsOfOneFolder) {
        const domain::SimpleNameCollation collation;
        const auto selection = domain::Selection::Create(kMixedSelection, collation);
        Assert::IsTrue(selection.ok());
        Assert::AreEqual(std::wstring(L"D:\\Work"), selection.value().parent());
        Assert::IsTrue(selection.value().names() == Paths{L"a.txt", L"b.jpg", L"Assets"});
    }

    TEST_METHOD(RejectsEmptyMixedParentsDuplicatesAndNonDrivePaths) {
        const domain::SimpleNameCollation collation;
        const auto codeOf = [&](Paths paths) {
            return domain::Selection::Create(std::move(paths), collation).error().code;
        };
        Assert::IsTrue(codeOf({}) == ErrorCode::EmptySelection);
        Assert::IsTrue(codeOf({L"D:\\Work\\a.txt", L"D:\\Other\\b.txt"}) == ErrorCode::MixedParents);
        Assert::IsTrue(codeOf({L"D:\\Work\\a.txt", L"D:\\work\\A.TXT"}) == ErrorCode::InvalidArgument);
        Assert::IsTrue(codeOf({L"\\\\server\\share\\a.txt"}) == ErrorCode::UnsupportedLocation);
    }

    TEST_METHOD(GuardRejectsMissingLinkedAndDisallowedItems) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        files.AddFolder(L"D:\\Work\\Link");
        files.MarkReparsePoint(L"D:\\Work\\Link");
        const auto codeOf = [&](Paths paths, app::AllowedItems allowed) {
            return app::ValidateSelection(std::move(paths), allowed, files, collation).error().code;
        };
        Assert::IsTrue(codeOf({L"D:\\Work\\gone.txt"}, app::AllowedItems::FilesAndFolders) ==
                       ErrorCode::ItemNotFound);
        Assert::IsTrue(codeOf({L"D:\\Work\\Link"}, app::AllowedItems::FilesAndFolders) ==
                       ErrorCode::UnsupportedLocation);
        Assert::IsTrue(codeOf({L"D:\\Work\\Assets"}, app::AllowedItems::FilesOnly) ==
                       ErrorCode::InvalidArgument);
    }
};

TEST_CLASS(GroupIntoNewFolderTests) {
public:
    TEST_METHOD(MovesSelectionIntoNewFolderKeepingSubtree) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        ScriptedPrompt prompt;
        prompt.folderNameAnswer = L"Project A";
        app::GroupIntoNewFolderUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute(kMixedSelection);

        Assert::IsTrue(report.ok());
        Assert::IsTrue(report.value().AllSucceeded());
        Assert::AreEqual(std::wstring(L"New Folder"), prompt.offeredFolderName);
        Assert::IsTrue(files.Exists(L"D:\\Work\\Project A\\a.txt"));
        Assert::IsTrue(files.Exists(L"D:\\Work\\Project A\\b.jpg"));
        Assert::IsTrue(files.Exists(L"D:\\Work\\Project A\\Assets\\logo.png"));
        Assert::IsFalse(files.Exists(L"D:\\Work\\a.txt"));
        Assert::IsFalse(files.Exists(L"D:\\Work\\Assets"));
        Assert::AreEqual(std::wstring(L"D:\\Work\\Project A\\a.txt"),
                         report.value().items[0].destination);
    }

    TEST_METHOD(SuggestsFreeDefaultName) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        files.AddFolder(L"D:\\Work\\new folder");
        files.AddFolder(L"D:\\Work\\New Folder (2)");
        ScriptedPrompt prompt;
        app::GroupIntoNewFolderUseCase useCase(files, files, prompt, collation);

        (void)useCase.Execute({L"D:\\Work\\a.txt"});

        Assert::AreEqual(std::wstring(L"New Folder (3)"), prompt.offeredFolderName);
    }

    TEST_METHOD(CancelChangesNothing) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        ScriptedPrompt prompt;
        app::GroupIntoNewFolderUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute(kMixedSelection);

        Assert::IsTrue(report.error().code == ErrorCode::Cancelled);
        Assert::AreEqual(0, files.mutationCount);
    }

    TEST_METHOD(InvalidSelectionNeverPrompts) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        ScriptedPrompt prompt;
        prompt.folderNameAnswer = L"X";
        app::GroupIntoNewFolderUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute({L"D:\\Work\\gone.txt"});

        Assert::IsTrue(report.error().code == ErrorCode::ItemNotFound);
        Assert::AreEqual(0, prompt.promptCount);
        Assert::AreEqual(0, files.mutationCount);
    }

    TEST_METHOD(ValidatorRejectsExistingAndInvalidNames) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        ScriptedPrompt prompt;
        app::GroupIntoNewFolderUseCase useCase(files, files, prompt, collation);

        // A misbehaving prompt returns a rejected name; the use case must still refuse it.
        for (const wchar_t* name : {L"ASSETS", L"a.txt", L"bad:name", L"NUL"}) {
            prompt.folderNameAnswer = name;
            const auto report = useCase.Execute({L"D:\\Work\\b.jpg"});
            Assert::IsTrue(prompt.folderNameProblem.has_value(), name);
            Assert::IsTrue(report.error().code == ErrorCode::InvalidName, name);
        }
        Assert::AreEqual(0, files.mutationCount);
    }

    TEST_METHOD(DestinationAppearingAfterPromptIsNotUsed) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        files.beforeCreateNewFolder = [&files] { files.AddFolder(L"D:\\Work\\Project A"); };
        ScriptedPrompt prompt;
        prompt.folderNameAnswer = L"Project A";
        app::GroupIntoNewFolderUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute(kMixedSelection);

        Assert::IsTrue(report.error().code == ErrorCode::NameCollision);
        Assert::IsTrue(files.Exists(L"D:\\Work\\a.txt"));
        Assert::IsFalse(files.Exists(L"D:\\Work\\Project A\\a.txt"));
        Assert::AreEqual(0, files.mutationCount);
    }

    TEST_METHOD(PartialFailureKeepsFolderAndReportsPerItem) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        files.failingSources.insert(FoldCase(L"D:\\Work\\b.jpg"));
        ScriptedPrompt prompt;
        prompt.folderNameAnswer = L"Project A";
        app::GroupIntoNewFolderUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute(kMixedSelection);

        Assert::IsTrue(report.ok());
        Assert::IsFalse(report.value().AllSucceeded());
        Assert::AreEqual(size_t{2}, report.value().Count(ItemStatus::Succeeded));
        Assert::AreEqual(size_t{1}, report.value().Count(ItemStatus::Failed));
        Assert::IsTrue(files.Exists(L"D:\\Work\\b.jpg"));
        Assert::IsTrue(files.Exists(L"D:\\Work\\Project A\\a.txt"));
    }

    TEST_METHOD(TotalFailureRemovesTheEmptyFolder) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        files.failingSources.insert(FoldCase(L"D:\\Work\\a.txt"));
        ScriptedPrompt prompt;
        prompt.folderNameAnswer = L"Project A";
        app::GroupIntoNewFolderUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute({L"D:\\Work\\a.txt"});

        Assert::IsTrue(report.ok());
        Assert::AreEqual(size_t{1}, report.value().Count(ItemStatus::Failed));
        Assert::IsFalse(files.Exists(L"D:\\Work\\Project A"));
        Assert::IsTrue(files.Exists(L"D:\\Work\\a.txt"));
    }
};

TEST_CLASS(DuplicateInPlaceTests) {
public:
    TEST_METHOD(CopiesFilesAndFolderTreesNextToOriginals) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        app::DuplicateInPlaceUseCase useCase(files, files, collation);

        const auto report = useCase.Execute({L"D:\\Work\\a.txt", L"D:\\Work\\Assets"});

        Assert::IsTrue(report.ok());
        Assert::IsTrue(report.value().AllSucceeded());
        Assert::IsTrue(files.Exists(L"D:\\Work\\a.txt"));
        Assert::IsTrue(files.Exists(L"D:\\Work\\a - Copy.txt"));
        Assert::IsTrue(files.Exists(L"D:\\Work\\Assets\\logo.png"));
        Assert::IsTrue(files.Exists(L"D:\\Work\\Assets - Copy\\logo.png"));
        Assert::AreEqual(std::wstring(L"D:\\Work\\a - Copy.txt"), report.value().items[0].destination);
    }

    TEST_METHOD(RepeatedDuplicateNeverOverwrites) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        app::DuplicateInPlaceUseCase useCase(files, files, collation);

        const auto first = useCase.Execute({L"D:\\Work\\a.txt"});
        const auto second = useCase.Execute({L"D:\\Work\\a.txt"});

        Assert::IsTrue(first.ok() && second.ok());
        Assert::IsTrue(first.value().items[0].destination != second.value().items[0].destination);
        Assert::IsTrue(files.Exists(L"D:\\Work\\a - Copy (2).txt"));
    }

    TEST_METHOD(InvalidSelectionChangesNothing) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        app::DuplicateInPlaceUseCase useCase(files, files, collation);

        const auto report = useCase.Execute({L"D:\\Work\\a.txt", L"D:\\Work\\Assets\\logo.png"});

        Assert::IsTrue(report.error().code == ErrorCode::MixedParents);
        Assert::AreEqual(0, files.mutationCount);
    }
};

TEST_CLASS(BulkRenameTests) {
public:
    TEST_METHOD(RenamesWithPostfixIndexKeepingExtension) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        ScriptedPrompt prompt;
        prompt.patternAnswer = domain::RenamePattern{};
        app::BulkRenameUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute({L"D:\\Work\\b.jpg", L"D:\\Work\\a.txt"});

        Assert::IsTrue(report.ok());
        Assert::IsTrue(report.value().AllSucceeded());
        Assert::IsTrue(files.Exists(L"D:\\Work\\a_01.txt"));
        Assert::IsTrue(files.Exists(L"D:\\Work\\b_02.jpg"));
        Assert::IsFalse(files.Exists(L"D:\\Work\\a.txt"));
        Assert::IsTrue(prompt.previewOfAnswer->ok());
        Assert::AreEqual(std::wstring(L"a_01.txt"), prompt.previewOfAnswer->value()[0].renamed);
    }

    TEST_METHOD(CancelChangesNothing) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        ScriptedPrompt prompt;
        app::BulkRenameUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute({L"D:\\Work\\a.txt"});

        Assert::IsTrue(report.error().code == ErrorCode::Cancelled);
        Assert::AreEqual(0, files.mutationCount);
    }

    TEST_METHOD(FolderInSelectionIsRejectedBeforePrompting) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        ScriptedPrompt prompt;
        prompt.patternAnswer = domain::RenamePattern{};
        app::BulkRenameUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute(kMixedSelection);

        Assert::IsTrue(report.error().code == ErrorCode::InvalidArgument);
        Assert::AreEqual(0, prompt.promptCount);
        Assert::AreEqual(0, files.mutationCount);
    }

    TEST_METHOD(CollisionWithUnselectedItemRefusesWholeBatch) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        files.AddFile(L"D:\\Work\\b_02.jpg");
        ScriptedPrompt prompt;
        prompt.patternAnswer = domain::RenamePattern{};
        app::BulkRenameUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute({L"D:\\Work\\a.txt", L"D:\\Work\\b.jpg"});

        Assert::IsFalse(prompt.previewOfAnswer->ok());
        Assert::IsTrue(report.error().code == ErrorCode::NameCollision);
        Assert::AreEqual(0, files.mutationCount);
        Assert::IsTrue(files.Exists(L"D:\\Work\\a.txt"));
    }

    TEST_METHOD(RerunOnNumberedBatchShiftsWithoutOverwriting) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files;
        files.AddFolder(L"D:\\P");
        for (const wchar_t* name : {L"D:\\P\\x_01.txt", L"D:\\P\\x_02.txt", L"D:\\P\\a.txt"}) {
            files.AddFile(name);
        }
        ScriptedPrompt prompt;
        domain::RenamePattern pattern;
        pattern.nameMask = L"x_[C]";
        prompt.patternAnswer = pattern;
        app::BulkRenameUseCase useCase(files, files, prompt, collation);

        const auto report =
            useCase.Execute({L"D:\\P\\x_01.txt", L"D:\\P\\x_02.txt", L"D:\\P\\a.txt"});

        Assert::IsTrue(report.ok());
        Assert::IsTrue(report.value().AllSucceeded());
        Assert::AreEqual(size_t{4}, files.EntryCount());
        Assert::IsTrue(files.Exists(L"D:\\P\\x_03.txt"));
        Assert::IsFalse(files.Exists(L"D:\\P\\a.txt"));
    }

    TEST_METHOD(FailureStopsRemainingSteps) {
        const domain::SimpleNameCollation collation;
        FakeFileSystem files = WorkFolder();
        files.failingSources.insert(FoldCase(L"D:\\Work\\a.txt"));
        ScriptedPrompt prompt;
        prompt.patternAnswer = domain::RenamePattern{};
        app::BulkRenameUseCase useCase(files, files, prompt, collation);

        const auto report = useCase.Execute({L"D:\\Work\\a.txt", L"D:\\Work\\b.jpg"});

        Assert::IsTrue(report.ok());
        Assert::AreEqual(size_t{1}, report.value().Count(ItemStatus::Failed));
        Assert::AreEqual(size_t{1}, report.value().Count(ItemStatus::NotAttempted));
        Assert::IsTrue(files.Exists(L"D:\\Work\\b.jpg"));
    }
};

}  // namespace et::tests
