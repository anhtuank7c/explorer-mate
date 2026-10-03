#pragma once

// wrl/module.h triggers C4324 (padding from an alignment specifier) at /W4; the layout is
// intentional in the SDK, so the warning is silenced for that header only.
#pragma warning(push)
#pragma warning(disable : 4324)
#include <wrl/module.h>
#pragma warning(pop)
