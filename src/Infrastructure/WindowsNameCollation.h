#pragma once

#include "Domain/NameCollation.h"

namespace et::infra {

// Name rules as Windows applies them: NTFS-style case-insensitive equality and the
// "logical" ordering File Explorer uses for its Name column.
class WindowsNameCollation final : public domain::INameCollation {
public:
    bool Equals(std::wstring_view left, std::wstring_view right) const override;
    bool NaturalLess(std::wstring_view left, std::wstring_view right) const override;
};

}  // namespace et::infra
