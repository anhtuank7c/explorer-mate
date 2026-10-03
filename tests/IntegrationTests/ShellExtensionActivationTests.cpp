#include <CppUnitTest.h>

#include <windows.h>

#include <shobjidl_core.h>
#include <wrl/client.h>

#include <filesystem>
#include <string>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::WRL::ComPtr;

namespace et::tests {

namespace {

// Must match src/ShellExtension/Commands.cpp and packaging/AppxManifest.xml.
constexpr CLSID kGroupCommand{
    0x417C229C, 0x5CC7, 0x4258, {0xB5, 0x9A, 0x60, 0x8F, 0x3F, 0xC6, 0x2D, 0x42}};
constexpr CLSID kRenameCommand{
    0x89086A14, 0x49B8, 0x4A64, {0xB6, 0x3E, 0x92, 0x40, 0x1D, 0x66, 0x52, 0xF8}};
constexpr CLSID kDuplicateCommand{
    0x166CCB3B, 0xAD13, 0x4C03, {0x8C, 0x5C, 0xFB, 0x2D, 0xBF, 0xB9, 0xC7, 0x3F}};

using GetClassObjectFn = HRESULT(STDAPICALLTYPE*)(REFCLSID, REFIID, LPVOID*);

std::filesystem::path ShellExtensionPath() {
    HMODULE self = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&ShellExtensionPath), &self);
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileNameW(self, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path() / L"ExplorerMate.Shell.dll";
}

// The DLL stays loaded for the whole test run; unloading a WRL module mid-run buys nothing.
GetClassObjectFn ClassObjectEntryPoint() {
    static const HMODULE library = LoadLibraryW(ShellExtensionPath().c_str());
    return library == nullptr ? nullptr
                              : reinterpret_cast<GetClassObjectFn>(
                                    GetProcAddress(library, "DllGetClassObject"));
}

std::wstring TitleOf(const CLSID& classId) {
    const GetClassObjectFn getClassObject = ClassObjectEntryPoint();
    ComPtr<IClassFactory> factory;
    ComPtr<IExplorerCommand> command;
    if (getClassObject == nullptr || FAILED(getClassObject(classId, IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreateInstance(nullptr, IID_PPV_ARGS(&command)))) {
        return {};
    }
    PWSTR raw = nullptr;
    if (FAILED(command->GetTitle(nullptr, &raw))) {
        return {};
    }
    std::wstring title(raw);
    CoTaskMemFree(raw);
    return title;
}

}  // namespace

// Activates the commands the way the shell host does, without any registration.
TEST_CLASS(ShellExtensionActivationTests) {
public:
    TEST_METHOD(EveryRegisteredClassCreatesItsCommand) {
        Assert::AreEqual(std::wstring(L"New folder with selection"), TitleOf(kGroupCommand));
        Assert::AreEqual(std::wstring(L"Bulk rename"), TitleOf(kRenameCommand));
        Assert::AreEqual(std::wstring(L"Duplicate"), TitleOf(kDuplicateCommand));
    }

    TEST_METHOD(CommandsAreEnabledWithoutSelection) {
        ComPtr<IClassFactory> factory;
        ComPtr<IExplorerCommand> command;
        Assert::IsTrue(SUCCEEDED(ClassObjectEntryPoint()(kRenameCommand, IID_PPV_ARGS(&factory))));
        Assert::IsTrue(SUCCEEDED(factory->CreateInstance(nullptr, IID_PPV_ARGS(&command))));

        EXPCMDSTATE state = ECS_HIDDEN;
        Assert::IsTrue(SUCCEEDED(command->GetState(nullptr, FALSE, &state)));
        Assert::IsTrue(state == ECS_ENABLED);
    }

    TEST_METHOD(UnknownClassIsRejected) {
        ComPtr<IClassFactory> factory;
        Assert::IsTrue(FAILED(ClassObjectEntryPoint()(CLSID_NULL, IID_PPV_ARGS(&factory))));
    }
};

}  // namespace et::tests
