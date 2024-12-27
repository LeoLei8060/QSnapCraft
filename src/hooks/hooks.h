#pragma once

#include <windows.h>

#ifdef HOOKS_EXPORTS
#define HOOKS_API __declspec(dllexport)
#else
#define HOOKS_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

HOOKS_API BOOL WINAPI InstallHooks();
HOOKS_API BOOL WINAPI UninstallHooks();

#ifdef __cplusplus
}
#endif

LRESULT CALLBACK CallWndProcHook(int code, WPARAM wParam, LPARAM lParam);
