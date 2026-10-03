#pragma once

#include <optional>

#include "Application/Settings.h"

namespace et::ui {

// Modal dialog for the hotkey settings. Returns the edited, validated settings, or nullopt
// when the user cancelled.
std::optional<app::Settings> EditSettings(const app::Settings& current);

}  // namespace et::ui
