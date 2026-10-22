#include "foregroundwindow.h"

#include <dwmapi.h>

QRect windowFrameBounds(HWND window)
{
    if (!window || !IsWindow(window)) {
        return {};
    }

    RECT          frame{};
    const HRESULT result = DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame));
    if (FAILED(result) && !GetWindowRect(window, &frame)) {
        return {};
    }

    const QRect bounds(frame.left, frame.top, frame.right - frame.left, frame.bottom - frame.top);
    return bounds.isValid() ? bounds : QRect();
}

HWND foregroundWindowExcludingCurrentProcess()
{
    const HWND window = GetForegroundWindow();
    if (!window) {
        return nullptr;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == GetCurrentProcessId()) {
        return nullptr;
    }
    return window;
}
