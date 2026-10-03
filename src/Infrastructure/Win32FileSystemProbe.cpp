#include "Infrastructure/Win32FileSystemProbe.h"

#include <windows.h>

#include <string_view>

#include "Infrastructure/ErrorText.h"

namespace et::infra {

namespace {

// Set on files whose content is not (fully) on disk, e.g. OneDrive online-only items.
constexpr DWORD kCloudAttributes = FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_RECALL_ON_OPEN |
                                   FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS;

class FindHandle {
public:
    explicit FindHandle(HANDLE handle) : handle_(handle) {}
    ~FindHandle() {
        if (valid()) {
            FindClose(handle_);
        }
    }
    FindHandle(const FindHandle&) = delete;
    FindHandle& operator=(const FindHandle&) = delete;

    bool valid() const { return handle_ != INVALID_HANDLE_VALUE; }
    HANDLE get() const { return handle_; }

private:
    HANDLE handle_;
};

bool IsDotEntry(std::wstring_view name) {
    return name == L"." || name == L"..";
}

}  // namespace

app::ItemInfo Win32FileSystemProbe::Inspect(const std::wstring& path) const {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
        return {};
    }
    app::ItemInfo info;
    info.exists = true;
    info.isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    info.isReparsePoint = (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    info.isCloudPlaceholder = (data.dwFileAttributes & kCloudAttributes) != 0;
    return info;
}

domain::Result<std::vector<std::wstring>> Win32FileSystemProbe::ListNames(
    const std::wstring& folder) const {
    std::wstring pattern = folder;
    if (!pattern.empty() && pattern.back() != L'\\') {
        pattern.push_back(L'\\');
    }
    pattern.push_back(L'*');

    WIN32_FIND_DATAW entry{};
    const FindHandle search(FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &entry,
                                             FindExSearchNameMatch, nullptr, 0));
    if (!search.valid()) {
        return domain::Error(domain::ErrorCode::OperationFailed,
                             L"Cannot list " + folder + L": " + DescribeWin32Error(GetLastError()));
    }

    std::vector<std::wstring> names;
    do {
        if (!IsDotEntry(entry.cFileName)) {
            names.emplace_back(entry.cFileName);
        }
    } while (FindNextFileW(search.get(), &entry));
    return names;
}

}  // namespace et::infra
