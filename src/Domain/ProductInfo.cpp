#include "Domain/ProductInfo.h"

#include "Domain/Version.h"

namespace et::domain {

std::wstring_view ProductName() {
    return L"Explorer Mate";
}

std::wstring_view ProductShortName() {
    return L"ExplorerMate";
}

#define ET_WIDEN_LITERAL(text) L##text
#define ET_WIDEN(text) ET_WIDEN_LITERAL(text)

std::wstring_view ProductVersion() {
    return ET_WIDEN(ET_VERSION_STRING);
}

std::wstring_view ProductAuthor() {
    return L"Tuan Nguyen (anhtuank7c)";
}

std::wstring_view ProductWebsiteUrl() {
    return L"https://meohamhoc.vn";
}

std::wstring_view ProductWebsiteLabel() {
    return L"meohamhoc.vn";
}

std::wstring_view ProductRepositoryUrl() {
    return L"https://github.com/anhtuank7c/explorer-mate";
}

std::wstring_view ProductRepositoryLabel() {
    return L"github.com/anhtuank7c/explorer-mate";
}

std::wstring_view ProductIssuesUrl() {
    return L"https://github.com/anhtuank7c/explorer-mate/issues";
}

}  // namespace et::domain
