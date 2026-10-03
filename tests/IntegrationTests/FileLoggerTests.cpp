#include <CppUnitTest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "Infrastructure/FileLogger.h"
#include "TempFixture.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

namespace {

std::string ReadAllBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

bool Contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

}  // namespace

TEST_CLASS(FileLoggerTests) {
public:
    TEST_METHOD(CreatesMissingDirectoryAndAppendsUtf8Lines) {
        const TempFixture fixture;
        const auto logPath = fixture.root() / L"logs" / L"test.log";
        infra::FileLogger logger(logPath.wstring());

        logger.Write(app::LogLevel::Info, L"D:\\T\u00E0i li\u1EC7u\\\u1EA3nh.jpg");
        logger.Write(app::LogLevel::Error, L"second");

        const std::string content = ReadAllBytes(logPath);
        Assert::IsTrue(Contains(content, "[INFO] D:\\T\xC3\xA0i li\xE1\xBB\x87u\\\xE1\xBA\xA3nh.jpg\r\n"));
        Assert::IsTrue(Contains(content, "[ERROR] second\r\n"));
        Assert::IsTrue(content.find("[INFO]") < content.find("[ERROR]"));
    }

    // Logged text can come from files and other processes; it must not forge extra entries.
    TEST_METHOD(MessageWithLineBreaksStaysOneEntry) {
        const TempFixture fixture;
        const auto logPath = fixture.root() / L"test.log";
        infra::FileLogger logger(logPath.wstring());

        logger.Write(app::LogLevel::Warning, L"first\r\n2026-01-01T00:00:00.000Z [INFO] forged");

        const std::string content = ReadAllBytes(logPath);
        Assert::AreEqual(size_t{1}, static_cast<size_t>(std::count(content.begin(), content.end(), '\n')));
        Assert::IsTrue(Contains(content, "[WARN] first  2026-01-01"));
    }

    TEST_METHOD(LargeLogIsRotatedNotGrownForever) {
        const TempFixture fixture;
        const auto logPath = fixture.root() / L"test.log";
        {
            std::ofstream big(logPath, std::ios::binary);
            big << std::string(1024 * 1024 + 1, 'x');
        }
        infra::FileLogger logger(logPath.wstring());

        logger.Write(app::LogLevel::Info, L"after rotation");

        Assert::IsTrue(std::filesystem::exists(logPath.wstring() + L".old"));
        Assert::IsTrue(std::filesystem::file_size(logPath) < 1024);
    }
};

}  // namespace et::tests
