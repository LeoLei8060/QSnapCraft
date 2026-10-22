#pragma once

#include <QRect>

#include <windows.h>

QRect windowFrameBounds(HWND window);
HWND  foregroundWindowExcludingCurrentProcess();
