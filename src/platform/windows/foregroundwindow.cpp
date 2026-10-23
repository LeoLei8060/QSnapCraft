#include "foregroundwindow.h"

#include <dwmapi.h>

#include <QPoint>

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

void focusWindowUnderPoint(const QPoint &global_position)
{
    POINT point{global_position.x(), global_position.y()};
    HWND  window = WindowFromPoint(point);
    if (window == nullptr) {
        return;
    }
    window = GetAncestor(window, GA_ROOT);
    if (window == nullptr || !IsWindowVisible(window)) {
        return;
    }

    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);
    if (process_id == GetCurrentProcessId()) {
        return;
    }

    const DWORD current_thread = GetCurrentThreadId();
    const DWORD target_thread = GetWindowThreadProcessId(window, nullptr);
    if (current_thread != target_thread) {
        AttachThreadInput(current_thread, target_thread, TRUE);
    }
    SetForegroundWindow(window);
    SetFocus(window);
    SetActiveWindow(window);
    if (current_thread != target_thread) {
        AttachThreadInput(current_thread, target_thread, FALSE);
    }
}
