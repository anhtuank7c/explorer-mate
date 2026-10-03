#include <CppUnitTest.h>

#include <filesystem>
#include <string>

#include "Infrastructure/RequestFileStore.h"
#include "TempFixture.h"
#include "TestSupport.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

namespace {
namespace fs = std::filesystem;
}

TEST_CLASS(RequestFileStoreTests) {
public:
    TEST_METHOD(PutThenTakeReturnsRequestAndDeletesFile) {
        const TempFixture fixture;
        const infra::RequestFileStore store((fixture.root() / L"requests").wstring());
        const app::ActionRequest request{app::ActionKind::GroupIntoNewFolder,
                                         {L"D:\\T\u00E0i li\u1EC7u\\\u1EA3nh.jpg", L"D:\\T\u00E0i li\u1EC7u\\b.txt"}};

        const auto path = store.Put(request);
        Assert::IsTrue(path.ok());
        Assert::IsTrue(fs::exists(path.value()));

        const auto taken = store.Take(path.value());
        Assert::IsTrue(taken.ok());
        Assert::IsTrue(taken.value().action == request.action);
        Assert::IsTrue(taken.value().items == request.items);
        Assert::IsFalse(fs::exists(path.value()));
    }

    TEST_METHOD(RefusesFilesOutsideTheStoreFolder) {
        const TempFixture fixture;
        const infra::RequestFileStore store((fixture.root() / L"requests").wstring());
        const fs::path outside = fixture.root() / L"elsewhere" / L"x.etreq";
        WriteText(outside, "ExMate-Request 1\naction=duplicate\nitem=D:\\a.txt\n");
        const fs::path traversal = fixture.root() / L"requests" / L".." / L"elsewhere" / L"x.etreq";

        Assert::IsFalse(store.Take(outside.wstring()).ok());
        Assert::IsFalse(store.Take(traversal.wstring()).ok());
        Assert::IsTrue(fs::exists(outside));
    }

    TEST_METHOD(RefusesWrongExtensionMissingFileAndInvalidContent) {
        const TempFixture fixture;
        const fs::path directory = fixture.root() / L"requests";
        const infra::RequestFileStore store(directory.wstring());
        WriteText(directory / L"note.txt", "ExMate-Request 1\naction=duplicate\nitem=D:\\a.txt\n");
        WriteText(directory / L"bad-utf8.etreq", "ExMate-Request 1\naction=duplicate\nitem=D:\\\xFF\xFE\n");
        WriteText(directory / L"garbage.etreq", "hello");

        Assert::IsFalse(store.Take((directory / L"note.txt").wstring()).ok());
        Assert::IsFalse(store.Take((directory / L"missing.etreq").wstring()).ok());
        Assert::IsFalse(store.Take((directory / L"bad-utf8.etreq").wstring()).ok());
        Assert::IsFalse(store.Take((directory / L"garbage.etreq").wstring()).ok());
        Assert::IsFalse(fs::exists(directory / L"garbage.etreq"));
        Assert::IsTrue(fs::exists(directory / L"note.txt"));
    }

    TEST_METHOD(RemoveStaleOnlyDeletesOldRequestFiles) {
        const TempFixture fixture;
        const fs::path directory = fixture.root() / L"requests";
        const infra::RequestFileStore store(directory.wstring());
        WriteText(directory / L"old.etreq", "x");
        WriteText(directory / L"keep.txt", "x");

        store.RemoveStale(3600);
        Assert::IsTrue(fs::exists(directory / L"old.etreq"));

        fs::last_write_time(directory / L"old.etreq",
                            fs::file_time_type::clock::now() - std::chrono::hours(2));
        fs::last_write_time(directory / L"keep.txt",
                            fs::file_time_type::clock::now() - std::chrono::hours(2));
        store.RemoveStale(3600);
        Assert::IsFalse(fs::exists(directory / L"old.etreq"));
        Assert::IsTrue(fs::exists(directory / L"keep.txt"));
    }
};

}  // namespace et::tests
