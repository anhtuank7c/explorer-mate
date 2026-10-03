#include "App/ActionRunner.h"

#include "Application/BulkRenameUseCase.h"
#include "Application/DuplicateInPlaceUseCase.h"
#include "Application/GroupIntoNewFolderUseCase.h"
#include "Infrastructure/Win32FileSystemProbe.h"
#include "Infrastructure/WindowsNameCollation.h"

namespace et::ui {

domain::Result<domain::OperationReport> RunAction(const app::ActionRequest& request,
                                                  app::IUserPrompt& prompt,
                                                  infra::OperationUi operationUi) {
    const infra::Win32FileSystemProbe probe;
    const infra::WindowsNameCollation collation;
    infra::ShellFileOperationGateway gateway(operationUi);

    switch (request.action) {
        case app::ActionKind::GroupIntoNewFolder:
            return app::GroupIntoNewFolderUseCase(probe, gateway, prompt, collation)
                .Execute(request.items);
        case app::ActionKind::BulkRename:
            return app::BulkRenameUseCase(probe, gateway, prompt, collation).Execute(request.items);
        case app::ActionKind::DuplicateInPlace:
            return app::DuplicateInPlaceUseCase(probe, gateway, collation).Execute(request.items);
    }
    return domain::Error(domain::ErrorCode::InvalidArgument, L"Unknown action.");
}

}  // namespace et::ui
