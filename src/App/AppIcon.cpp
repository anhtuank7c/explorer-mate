#include "App/AppIcon.h"

#include "App/resource.h"

namespace et::ui {

namespace {

HICON LoadAppIcon(int widthMetric, int heightMetric) {
    return static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APP),
                                         IMAGE_ICON, GetSystemMetrics(widthMetric),
                                         GetSystemMetrics(heightMetric), LR_SHARED));
}

}  // namespace

HICON SmallAppIcon() {
    return LoadAppIcon(SM_CXSMICON, SM_CYSMICON);
}

void ApplyAppIcon(HWND window) {
    SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(SmallAppIcon()));
    SendMessageW(window, WM_SETICON, ICON_BIG,
                 reinterpret_cast<LPARAM>(LoadAppIcon(SM_CXICON, SM_CYICON)));
}

}  // namespace et::ui
