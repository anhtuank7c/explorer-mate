#include <CppUnitTest.h>

#include <string>

#include "Domain/Result.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

TEST_CLASS(ResultTests) {
public:
    TEST_METHOD(HoldsValue) {
        const domain::Result<std::wstring> result(std::wstring(L"ok"));
        Assert::IsTrue(result.ok());
        Assert::AreEqual(std::wstring(L"ok"), result.value());
    }

    TEST_METHOD(HoldsError) {
        const domain::Result<int> result(domain::Error(domain::ErrorCode::Cancelled, L"stopped"));
        Assert::IsFalse(result.ok());
        Assert::IsTrue(result.error().code == domain::ErrorCode::Cancelled);
        Assert::AreEqual(std::wstring(L"stopped"), result.error().message);
    }
};

}  // namespace et::tests
