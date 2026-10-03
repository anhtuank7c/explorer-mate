#pragma once

#include <string>

#include "Application/Settings.h"
#include "Domain/Result.h"

namespace et::infra {

// Persists the agent's settings as UTF-8 text.
class SettingsFile {
public:
    // `path` is normally <ProductDataDirectory>\settings.txt.
    explicit SettingsFile(std::wstring path);

    // Defaults when the file does not exist yet. An unreadable or invalid file is an error:
    // silently replacing it with defaults would discard the user's shortcuts.
    domain::Result<app::Settings> Load() const;

    domain::Status Save(const app::Settings& settings) const;

private:
    std::wstring path_;
};

}  // namespace et::infra
