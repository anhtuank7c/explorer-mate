#pragma once

#include <windows.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace et::tests {

// A private directory under %TEMP%\ExMate.Tests. It is only deleted when the ownership
// marker written at creation is still present, so a wrong path can never be wiped.
class TempFixture {
public:
    TempFixture() {
        root_ = std::filesystem::temp_directory_path() / L"ExMate.Tests" /
                (std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()) +
                 L"-" + std::to_wstring(++NextId()));
        std::filesystem::create_directories(root_);
        // %TEMP% is often an 8.3 short path; the Shell reports long paths.
        root_ = std::filesystem::canonical(root_);
        std::ofstream(root_ / kMarkerName) << "ExMate test fixture";
    }

    ~TempFixture() {
        std::error_code ignored;
        if (std::filesystem::exists(root_ / kMarkerName, ignored)) {
            std::filesystem::remove_all(root_, ignored);
        }
    }

    TempFixture(const TempFixture&) = delete;
    TempFixture& operator=(const TempFixture&) = delete;

    const std::filesystem::path& root() const { return root_; }

private:
    static constexpr const wchar_t* kMarkerName = L".exmate-fixture";

    static std::atomic<unsigned>& NextId() {
        static std::atomic<unsigned> next{0};
        return next;
    }

    std::filesystem::path root_;
};

}  // namespace et::tests
