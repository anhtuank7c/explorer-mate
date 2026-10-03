#include <CppUnitTest.h>

#include <string>

#include "Domain/ProductInfo.h"
#include "Domain/Result.h"
#include "Domain/Version.h"

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

TEST_CLASS(VersionTests) {
public:
    // Version.h states the version twice (numbers for the resources, text for everything
    // else); they must not drift apart.
    TEST_METHOD(VersionStringMatchesItsParts) {
        const std::wstring expected = std::to_wstring(ET_VERSION_MAJOR) + L"." +
                                      std::to_wstring(ET_VERSION_MINOR) + L"." +
                                      std::to_wstring(ET_VERSION_PATCH);
        Assert::AreEqual(expected, std::wstring(domain::ProductVersion()));
    }
};

}  // namespace et::tests
