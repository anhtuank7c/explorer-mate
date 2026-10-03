#include <CppUnitTest.h>

#include <string>

#include "Domain/RenameMask.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

namespace {

// IMG_2024.jpg, counter 07, inside folder "Da Lat".
const domain::MaskContext kContext{L"IMG_2024", L"jpg", L"07", L"Da Lat"};

std::wstring Expand(const wchar_t* mask) {
    const auto expanded = domain::ExpandMask(mask, kContext);
    Assert::IsTrue(expanded.ok(), mask);
    return expanded.value();
}

}  // namespace

TEST_CLASS(RenameMaskTests) {
public:
    TEST_METHOD(ExpandsBasicPlaceholders) {
        Assert::AreEqual(std::wstring(L"IMG_2024_07"), Expand(L"[N]_[C]"));
        Assert::AreEqual(std::wstring(L"jpg"), Expand(L"[E]"));
        Assert::AreEqual(std::wstring(L"Da Lat - 07"), Expand(L"[P] - [C]"));
        Assert::AreEqual(std::wstring(L"07 IMG_2024"), Expand(L"[C] [N]"));
    }

    TEST_METHOD(CopiesLiteralTextIncludingVietnamese) {
        Assert::AreEqual(std::wstring(L"Ảnh 07"), Expand(L"Ảnh [C]"));
        Assert::AreEqual(std::wstring(L"plain"), Expand(L"plain"));
        Assert::AreEqual(std::wstring(L""), Expand(L""));
    }

    TEST_METHOD(ExpandsCharacterRanges) {
        Assert::AreEqual(std::wstring(L"IMG"), Expand(L"[N1-3]"));
        Assert::AreEqual(std::wstring(L"2024"), Expand(L"[N5-]"));
        Assert::AreEqual(std::wstring(L"M"), Expand(L"[N2]"));
        Assert::AreEqual(std::wstring(L"_20"), Expand(L"[N4,3]"));
        Assert::AreEqual(std::wstring(L"jp"), Expand(L"[E1-2]"));
        Assert::AreEqual(std::wstring(L"Da"), Expand(L"[P1-2]"));
    }

    TEST_METHOD(ClipsRangesBeyondTheEnd) {
        Assert::AreEqual(std::wstring(L"2024"), Expand(L"[N5-99]"));
        Assert::AreEqual(std::wstring(L""), Expand(L"[N50-60]"));
        Assert::AreEqual(std::wstring(L""), Expand(L"[N50]"));
        Assert::AreEqual(std::wstring(L"4"), Expand(L"[N8,10]"));
    }

    TEST_METHOD(SupportsLiteralBrackets) {
        Assert::AreEqual(std::wstring(L"[07] IMG_2024"), Expand(L"[[][C][]] [N]"));
    }

    TEST_METHOD(RejectsMalformedMasks) {
        for (const wchar_t* mask : {L"[X]", L"[N", L"N]", L"[]", L"[N0]", L"[N5-2]", L"[N1-2-3]",
                                    L"[Nabc]", L"[C2]", L"[n]", L"[N1,]", L"[N-3]"}) {
            Assert::IsFalse(domain::ExpandMask(mask, kContext).ok(), mask);
            Assert::IsTrue(domain::ValidateMask(mask).has_value(), mask);
        }
        Assert::IsFalse(domain::ValidateMask(L"[N]_[C]").has_value());
    }
};

}  // namespace et::tests
