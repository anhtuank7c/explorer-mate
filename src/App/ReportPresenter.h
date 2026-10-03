#pragma once

#include <string>

#include "Domain/Error.h"
#include "Domain/OperationReport.h"

namespace et::ui {

// Text shown to the user after a command. Empty when there is nothing worth interrupting
// them for: everything succeeded, or they cancelled.
std::wstring DescribeProblems(const domain::OperationReport& report);

// Message box with the product name as caption.
void ShowProblem(const std::wstring& message);

}  // namespace et::ui
