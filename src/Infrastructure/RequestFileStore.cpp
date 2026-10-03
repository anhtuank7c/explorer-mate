#include "Infrastructure/RequestFileStore.h"

#include <windows.h>

#include <objbase.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

#include "Infrastructure/Utf8.h"

namespace et::infra {

namespace {

namespace fs = std::filesystem;

constexpr const wchar_t* kExtension = L".etreq";
constexpr uintmax_t kMaxFileBytes = 64ull * 1024 * 1024;

domain::Error StoreError(const std::wstring& message) {
    return domain::Error(domain::ErrorCode::OperationFailed, message);
}

std::wstring NewFileName() {
    GUID guid{};
    if (FAILED(CoCreateGuid(&guid))) {
        return {};
    }
    wchar_t text[64]{};
    if (StringFromGUID2(guid, text, static_cast<int>(std::size(text))) == 0) {
        return {};
    }
    std::wstring name(text);
    // "{...}" -> "..." so the name needs no quoting anywhere.
    return name.substr(1, name.size() - 2) + kExtension;
}

// True when `path` names a request file directly inside `directory`.
bool IsInside(const fs::path& directory, const fs::path& path) {
    std::error_code error;
    const fs::path canonicalDirectory = fs::weakly_canonical(directory, error);
    if (error) {
        return false;
    }
    const fs::path canonicalParent = fs::weakly_canonical(path.parent_path(), error);
    return !error && path.extension() == kExtension &&
           CompareStringOrdinal(canonicalDirectory.c_str(), -1, canonicalParent.c_str(), -1, TRUE) ==
               CSTR_EQUAL;
}

}  // namespace

RequestFileStore::RequestFileStore(std::wstring directory) : directory_(std::move(directory)) {}

domain::Result<std::wstring> RequestFileStore::Put(const app::ActionRequest& request) const {
    std::error_code error;
    fs::create_directories(directory_, error);
    const std::wstring name = NewFileName();
    if (error || name.empty()) {
        return StoreError(L"Cannot prepare the request folder: " + directory_);
    }

    // Exact encoding only: a replaced character would make the worker act on another file.
    const auto bytes = ToUtf8Exact(app::SerializeRequest(request));
    if (!bytes) {
        return domain::Error(domain::ErrorCode::UnsupportedLocation,
                             L"A selected item has a name that cannot be passed on safely.");
    }
    const fs::path path = fs::path(directory_) / name;
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(bytes->data(), static_cast<std::streamsize>(bytes->size()));
    file.close();
    if (!file) {
        fs::remove(path, error);
        return StoreError(L"Cannot write the request file: " + path.wstring());
    }
    return path.wstring();
}

domain::Result<app::ActionRequest> RequestFileStore::Take(const std::wstring& path) const {
    const fs::path file(path);
    if (!IsInside(directory_, file)) {
        return domain::Error(domain::ErrorCode::InvalidArgument,
                             L"The request file is outside the request folder.");
    }

    std::error_code error;
    const uintmax_t size = fs::file_size(file, error);
    if (error) {
        return StoreError(L"Cannot read the request file: " + path);
    }

    std::string bytes;
    if (size <= kMaxFileBytes) {
        std::ifstream input(file, std::ios::binary);
        bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }
    // One-shot: the file is removed whether or not its content turns out to be usable.
    fs::remove(file, error);

    if (size > kMaxFileBytes) {
        return domain::Error(domain::ErrorCode::InvalidArgument, L"The request file is too large.");
    }
    const auto text = FromUtf8(bytes);
    if (!text) {
        return domain::Error(domain::ErrorCode::InvalidArgument,
                             L"The request file is not valid UTF-8.");
    }
    return app::ParseRequest(*text);
}

void RequestFileStore::RemoveStale(unsigned maxAgeSeconds) const {
    // Best-effort housekeeping: every call uses the error_code overloads so nothing throws.
    std::error_code error;
    const auto now = fs::file_time_type::clock::now();
    for (fs::directory_iterator entry(directory_, error), end; !error && entry != end;
         entry.increment(error)) {
        if (entry->path().extension() != kExtension) {
            continue;
        }
        std::error_code entryError;
        const auto written = entry->last_write_time(entryError);
        if (!entryError && now - written > std::chrono::seconds(maxAgeSeconds)) {
            fs::remove(entry->path(), entryError);
        }
    }
}

}  // namespace et::infra
