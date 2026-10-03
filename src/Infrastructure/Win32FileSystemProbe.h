#pragma once

#include "Application/Ports/IFileSystemProbe.h"

namespace et::infra {

class Win32FileSystemProbe final : public app::IFileSystemProbe {
public:
    app::ItemInfo Inspect(const std::wstring& path) const override;
    domain::Result<std::vector<std::wstring>> ListNames(const std::wstring& folder) const override;
};

}  // namespace et::infra
