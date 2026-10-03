#include <CppUnitTest.h>

#include <string>

#include "Domain/FolderName.h"
#include "Domain/ItemName.h"
#include "Domain/NameCollation.h"
#include "Domain/PathText.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

TEST_CLASS(ItemNameTests) {
public:
    TEST_METHOD(SplitsAtLastDot) {
        const auto parts = domain::SplitStemAndExtension(L"archive.tar.gz");
        Assert::AreEqual(std::wstring(L"archive.tar"), parts.stem);
        Assert::AreEqual(std::wstring(L".gz"), parts.extension);
    }

    TEST_METHOD(DotfileHasNoExtension) {
        const auto parts = domain::SplitStemAndExtension(L".gitignore");
        Assert::AreEqual(std::wstring(L".gitignore"), parts.stem);
        Assert::IsTrue(parts.extension.empty());
    }

    TEST_METHOD(NameWithoutDotHasNoExtension) {
        const auto parts = domain::SplitStemAndExtension(L"Makefile");
        Assert::AreEqual(std::wstring(L"Makefile"), parts.stem);
        Assert::IsTrue(parts.extension.empty());
    }

    TEST_METHOD(AcceptsOrdinaryAndVietnameseNames) {
        Assert::IsFalse(domain::ValidateItemName(L"Project A").has_value());
        Assert::IsFalse(domain::ValidateItemName(L"T\u00E0i li\u1EC7u 2026").has_value());
        Assert::IsFalse(domain::ValidateItemName(L".config").has_value());
        Assert::IsFalse(domain::ValidateItemName(L"v1.2.3").has_value());
    }

    TEST_METHOD(RejectsEmptyAndForbiddenCharacters) {
        Assert::IsTrue(domain::ValidateItemName(L"").has_value());
        for (const wchar_t* name : {L"a\\b", L"a/b", L"a:b", L"a*b", L"a?b", L"a\"b", L"a<b",
                                    L"a>b", L"a|b", L"a\tb"}) {
            Assert::IsTrue(domain::ValidateItemName(name).has_value(), name);
        }
    }

    TEST_METHOD(RejectsLeadingSpaceAndTrailingDotOrSpace) {
        Assert::IsTrue(domain::ValidateItemName(L"name.").has_value());
        Assert::IsTrue(domain::ValidateItemName(L"name ").has_value());
        Assert::IsTrue(domain::ValidateItemName(L" name").has_value());
    }

    TEST_METHOD(RejectsReservedDeviceNames) {
        for (const wchar_t* name : {L"CON", L"nul", L"Com1", L"LPT9", L"NUL.txt", L"aux.tar.gz",
                                    L"COM0", L"lpt0", L"CONIN$", L"conout$", L"COM¹",
                                    L"LPT³.txt"}) {
            Assert::IsTrue(domain::ValidateItemName(name).has_value(), name);
        }
        Assert::IsFalse(domain::ValidateItemName(L"CONSOLE").has_value());
        Assert::IsFalse(domain::ValidateItemName(L"COM10").has_value());
    }

    TEST_METHOD(RejectsOverlongName) {
        Assert::IsFalse(domain::ValidateItemName(std::wstring(255, L'a')).has_value());
        Assert::IsTrue(domain::ValidateItemName(std::wstring(256, L'a')).has_value());
    }

    TEST_METHOD(FolderNameWrapsValidation) {
        Assert::IsTrue(domain::FolderName::Create(L"Project A").ok());
        const auto invalid = domain::FolderName::Create(L"a/b");
        Assert::IsFalse(invalid.ok());
        Assert::IsTrue(invalid.error().code == domain::ErrorCode::InvalidName);
    }
};

TEST_CLASS(PathTextTests) {
public:
    TEST_METHOD(SplitsParentAndName) {
        Assert::AreEqual(std::wstring(L"D:\\Work"), std::wstring(domain::ParentOf(L"D:\\Work\\a.txt")));
        Assert::AreEqual(std::wstring(L"a.txt"), std::wstring(domain::NameOf(L"D:\\Work\\a.txt")));
    }

    TEST_METHOD(ParentOfRootItemKeepsSeparator) {
        Assert::AreEqual(std::wstring(L"D:\\"), std::wstring(domain::ParentOf(L"D:\\a.txt")));
        Assert::AreEqual(std::wstring(L"D:\\a.txt"), domain::JoinPath(L"D:\\", L"a.txt"));
        Assert::AreEqual(std::wstring(L"D:\\Work\\a.txt"), domain::JoinPath(L"D:\\Work", L"a.txt"));
    }

    TEST_METHOD(RecognisesDriveAbsoluteItemPaths) {
        Assert::IsTrue(domain::IsDriveAbsoluteItemPath(L"D:\\a.txt"));
        Assert::IsTrue(domain::IsDriveAbsoluteItemPath(L"c:\\Users\\me\\file"));
        for (const wchar_t* path : {L"", L"D:\\", L"D:", L"\\\\server\\share\\a.txt", L"a.txt",
                                    L"D:/a.txt", L"D:\\Work\\", L"D:\\Work\\\\a.txt",
                                    L"::{645FF040-5081-101B-9F08-00AA002F954E}",
                                    L"\\\\?\\D:\\a.txt"}) {
            Assert::IsFalse(domain::IsDriveAbsoluteItemPath(path), path);
        }
    }

    // Texts that Windows would silently normalise to another item, or that address something
    // other than a plain file: two such texts could name the same file.
    TEST_METHOD(RejectsPathsThatDoNotNameExactlyOneItem) {
        for (const wchar_t* path : {L"D:\\Work\\..", L"D:\\Work\\..\\a.txt", L"D:\\Work\\.\\a.txt",
                                    L"D:\\Work\\a.txt.", L"D:\\Work\\a.txt ", L"D:\\Work.\\a.txt",
                                    L"D:\\Work\\a.txt:stream", L"D:\\Work\\a.txt::$DATA",
                                    L"D:\\Work\\a\tb.txt", L"D:\\Work\\a\nb.txt"}) {
            Assert::IsFalse(domain::IsDriveAbsoluteItemPath(path), path);
        }
        const std::wstring withNul = std::wstring(L"D:\\Work\\a") + L'\0' + L"b.txt";
        Assert::IsFalse(domain::IsDriveAbsoluteItemPath(withNul));
        Assert::IsTrue(domain::IsDriveAbsoluteItemPath(L"D:\\Work\\.gitignore"));
        Assert::IsTrue(domain::IsDriveAbsoluteItemPath(L"D:\\Work\\v1.2\\a b.txt"));
    }
};

TEST_CLASS(NameCollationTests) {
public:
    TEST_METHOD(EqualsIgnoresAsciiCase) {
        const domain::SimpleNameCollation collation;
        Assert::IsTrue(collation.Equals(L"Photo.JPG", L"photo.jpg"));
        Assert::IsFalse(collation.Equals(L"photo.jpg", L"photo.jpeg"));
    }

    TEST_METHOD(OrdersDigitRunsNumerically) {
        const domain::SimpleNameCollation collation;
        Assert::IsTrue(collation.NaturalLess(L"IMG_7.jpg", L"IMG_12.jpg"));
        Assert::IsFalse(collation.NaturalLess(L"IMG_12.jpg", L"IMG_7.jpg"));
        Assert::IsTrue(collation.NaturalLess(L"a2", L"a10"));
        Assert::IsTrue(collation.NaturalLess(L"a", L"a1"));
        Assert::IsTrue(collation.NaturalLess(L"apple", L"Banana"));
    }

    TEST_METHOD(OrderingIsStrict) {
        const domain::SimpleNameCollation collation;
        Assert::IsFalse(collation.NaturalLess(L"same", L"same"));
        Assert::IsTrue(collation.NaturalLess(L"A", L"a") != collation.NaturalLess(L"a", L"A"));
    }
};

}  // namespace et::tests
