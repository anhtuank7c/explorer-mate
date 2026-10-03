#include "Domain/ProductInfo.h"

namespace et::domain {

std::wstring_view ProductName() {
    return L"Explorer Mate";
}

std::wstring_view ProductShortName() {
    return L"ExplorerMate";
}

std::wstring_view ProductVersion() {
    return L"0.1.0";
}

std::wstring_view ProductAuthor() {
    return L"Tuan Nguyen";
}

std::wstring_view ProductWebsiteUrl() {
    return L"https://meohamhoc.vn";
}

std::wstring_view ProductWebsiteLabel() {
    return L"meohamhoc.vn";
}

}  // namespace et::domain
