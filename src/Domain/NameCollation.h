#pragma once

#include <string_view>

namespace et::domain {

// How item names compare. The platform decides what "same name" and "Explorer order" mean,
// so the rules are injected; Infrastructure provides the Windows-accurate implementation.
class INameCollation {
public:
    virtual ~INameCollation() = default;

    // True when the filesystem would treat both names as the same entry.
    virtual bool Equals(std::wstring_view left, std::wstring_view right) const = 0;

    // Strict weak ordering matching the order the user sees in the file list.
    virtual bool NaturalLess(std::wstring_view left, std::wstring_view right) const = 0;
};

// Portable approximation: ASCII case-insensitive, digit runs compared by numeric value.
class SimpleNameCollation final : public INameCollation {
public:
    bool Equals(std::wstring_view left, std::wstring_view right) const override;
    bool NaturalLess(std::wstring_view left, std::wstring_view right) const override;
};

}  // namespace et::domain
