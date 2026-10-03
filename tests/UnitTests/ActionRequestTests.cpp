#include <CppUnitTest.h>

#include <string>

#include "Application/ActionRequest.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

TEST_CLASS(ActionRequestTests) {
public:
    TEST_METHOD(RoundTripsActionAndItems) {
        const app::ActionRequest request{app::ActionKind::BulkRename,
                                         {L"D:\\T\u00E0i li\u1EC7u\\a b.txt", L"D:\\T\u00E0i li\u1EC7u\\c=d.txt"}};
        const auto parsed = app::ParseRequest(app::SerializeRequest(request));
        Assert::IsTrue(parsed.ok());
        Assert::IsTrue(parsed.value().action == app::ActionKind::BulkRename);
        Assert::IsTrue(parsed.value().items == request.items);
    }

    TEST_METHOD(AcceptsWindowsLineEndings) {
        const auto parsed = app::ParseRequest(
            L"ExMate-Request 1\r\naction=duplicate\r\nitem=D:\\a.txt\r\n");
        Assert::IsTrue(parsed.ok());
        Assert::AreEqual(std::wstring(L"D:\\a.txt"), parsed.value().items[0]);
    }

    TEST_METHOD(RejectsMalformedRequests) {
        for (const wchar_t* text : {
                 L"",
                 L"ExMate-Request 2\naction=duplicate\nitem=D:\\a.txt\n",
                 L"ExMate-Request 1\nitem=D:\\a.txt\n",
                 L"ExMate-Request 1\naction=duplicate\n",
                 L"ExMate-Request 1\naction=format\nitem=D:\\a.txt\n",
                 L"ExMate-Request 1\naction=duplicate\naction=group\nitem=D:\\a.txt\n",
                 L"ExMate-Request 1\naction=duplicate\nitem=a.txt\n",
                 L"ExMate-Request 1\naction=duplicate\nitem=\\\\server\\share\\a.txt\n",
                 L"ExMate-Request 1\naction=duplicate\nitem=D:\\a.txt\nrun=calc.exe\n",
             }) {
            Assert::IsFalse(app::ParseRequest(text).ok(), text);
        }
    }
};

}  // namespace et::tests
