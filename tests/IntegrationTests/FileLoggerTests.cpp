#include <CppUnitTest.h>

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
};

}  // namespace et::tests
