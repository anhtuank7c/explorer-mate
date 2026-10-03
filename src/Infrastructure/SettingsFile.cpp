#include "Infrastructure/SettingsFile.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

#include "Infrastructure/Utf8.h"

namespace et::infra {

namespace {

namespace fs = std::filesystem;

domain::Error FileError(const std::wstring& message) {
    return domain::Error(domain::ErrorCode::OperationFailed, message);
}

}  // namespace

SettingsFile::SettingsFile(std::wstring path) : path_(std::move(path)) {}

domain::Result<app::Settings> SettingsFile::Load() const {
    std::error_code error;
    if (!fs::exists(path_, error)) {
        return app::Settings::Defaults();
    }
    std::ifstream file(fs::path(path_), std::ios::binary);
    if (!file) {
        return FileError(L"Cannot read " + path_);
    }
    const std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const auto text = FromUtf8(bytes);
    if (!text) {
        return FileError(path_ + L" is not valid UTF-8.");
    }
    return app::ParseSettings(*text);
}

domain::Status SettingsFile::Save(const app::Settings& settings) const {
    if (const auto invalid = app::ValidateSettings(settings)) {
        return *invalid;
    }
    std::error_code error;
    fs::create_directories(fs::path(path_).parent_path(), error);

    // Write to a sibling file first so a crash never leaves a half-written settings file.
    const fs::path temporary = fs::path(path_ + L".tmp");
    const std::string bytes = ToUtf8(app::SerializeSettings(settings));
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!file) {
            return FileError(L"Cannot write " + temporary.wstring());
        }
    }
    fs::rename(temporary, path_, error);
    if (error) {
        fs::remove(temporary, error);
        return FileError(L"Cannot replace " + path_);
    }
    return domain::Unit{};
}

}  // namespace et::infra
