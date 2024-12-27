#include "hooks.h"

HHOOK             g_hCallWndProcHook = NULL;
HOOKS_API LPCTSTR g_cursorName = IDC_CROSS;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

BOOL WINAPI InstallHooks()
{
    if (!g_hCallWndProcHook) {
        g_hCallWndProcHook = SetWindowsHookEx(WH_CALLWNDPROC,
                                              CallWndProcHook,
                                              GetModuleHandle(TEXT("hooks")),
                                              0);
    }
    return (g_hCallWndProcHook != NULL);
}

BOOL WINAPI UninstallHooks()
{
    BOOL result = TRUE;
    if (g_hCallWndProcHook) {
        result = UnhookWindowsHookEx(g_hCallWndProcHook);
        g_hCallWndProcHook = NULL;
    }
    return result;
}

LRESULT CALLBACK CallWndProcHook(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0) {
        CWPSTRUCT *msg = (CWPSTRUCT *) lParam;
        if (msg->message == WM_SETCURSOR) {
            SetCursor(LoadCursor(NULL, g_cursorName));
            return TRUE;
        }
    }
    return CallNextHookEx(g_hCallWndProcHook, code, wParam, lParam);
}
