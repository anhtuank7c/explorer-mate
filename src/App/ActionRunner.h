#pragma once

#include "Application/ActionRequest.h"
#include "Application/Ports/IUserPrompt.h"
#include "Domain/OperationReport.h"
#include "Domain/Result.h"
#include "Infrastructure/ShellFileOperationGateway.h"

namespace et::ui {

// Composition root for one request: wires the Windows adapters into the matching use case
// and runs it. The calling thread must be in a COM single-threaded apartment.
domain::Result<domain::OperationReport> RunAction(const app::ActionRequest& request,
                                                  app::IUserPrompt& prompt,
                                                  infra::OperationUi operationUi);

}  // namespace et::ui
