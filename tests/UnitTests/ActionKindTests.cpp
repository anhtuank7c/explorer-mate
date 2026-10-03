#include <CppUnitTest.h>

#include "Application/ActionKind.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

TEST_CLASS(ActionKindTests) {
public:
    TEST_METHOD(ParsesEveryWireName) {
        for (const auto action : {app::ActionKind::GroupIntoNewFolder, app::ActionKind::BulkRename,
                                  app::ActionKind::DuplicateInPlace}) {
            const auto parsed = app::ParseActionKind(app::ToWireName(action));
            Assert::IsTrue(parsed.ok());
            Assert::IsTrue(parsed.value() == action);
        }
    }

    TEST_METHOD(RejectsUnknownWireName) {
        const auto parsed = app::ParseActionKind(L"delete");
        Assert::IsFalse(parsed.ok());
        Assert::IsTrue(parsed.error().code == domain::ErrorCode::InvalidArgument);
    }

    TEST_METHOD(WireNamesAreCaseSensitive) {
        Assert::IsFalse(app::ParseActionKind(L"Duplicate").ok());
    }
};

}  // namespace et::tests
