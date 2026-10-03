#pragma once

#include <string>

namespace et::infra {

// System message for an HRESULT followed by its code, e.g. "Access is denied. (0x80070005)".
std::wstring DescribeHresult(long result);

// Same for a Win32 error code such as GetLastError().
std::wstring DescribeWin32Error(unsigned long error);

}  // namespace et::infra
