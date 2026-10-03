#include "ShellExtension/ExplorerCommandBase.h"
#include "ShellExtension/WrlModule.h"

// The three context-menu commands. Each CLSID must match packaging/AppxManifest.xml and
// tests/IntegrationTests/ShellExtensionActivationTests.cpp.

namespace et::ui {

class __declspec(uuid("417C229C-5CC7-4258-B59A-608F3FC62D42")) GroupIntoNewFolderCommand final
    : public ExplorerCommandBase {
protected:
    std::wstring_view Title() const override { return L"New folder with selection"; }
    app::ActionKind Action() const override { return app::ActionKind::GroupIntoNewFolder; }
};

class __declspec(uuid("89086A14-49B8-4A64-B63E-92401D6652F8")) BulkRenameCommand final
    : public ExplorerCommandBase {
protected:
    std::wstring_view Title() const override { return L"Bulk rename"; }
    app::ActionKind Action() const override { return app::ActionKind::BulkRename; }
    bool AcceptsFolders() const override { return false; }
};

class __declspec(uuid("166CCB3B-AD13-4C03-8C5C-FB2DBFB9C73F")) DuplicateInPlaceCommand final
    : public ExplorerCommandBase {
protected:
    std::wstring_view Title() const override { return L"Duplicate"; }
    app::ActionKind Action() const override { return app::ActionKind::DuplicateInPlace; }
};

CoCreatableClass(GroupIntoNewFolderCommand);
CoCreatableClass(BulkRenameCommand);
CoCreatableClass(DuplicateInPlaceCommand);

}  // namespace et::ui
