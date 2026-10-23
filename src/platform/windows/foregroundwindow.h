#pragma once

#include <QPoint>
#include <QRect>

#include <windows.h>

QRect windowFrameBounds(HWND window);
HWND  foregroundWindowExcludingCurrentProcess();
void  focusWindowUnderPoint(const QPoint &global_position);
