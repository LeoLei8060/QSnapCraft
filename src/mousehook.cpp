#include "mousehook.h"

MouseHook *MouseHook::instance = nullptr;

MouseHook::MouseHook(QObject *parent)
    : QObject(parent)
    , m_hook(NULL)
{
    instance = this;
}

MouseHook::~MouseHook()
{
    uninstall();
}

void MouseHook::install()
{
    if (m_hook)
        return;

    m_hook = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, GetModuleHandle(NULL), 0);
}

void MouseHook::uninstall()
{
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = NULL;
    }
}

LRESULT CALLBACK MouseHook::mouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && instance) {
        MSLLHOOKSTRUCT *pMouseStruct = (MSLLHOOKSTRUCT *) lParam;
        POINT           pt = pMouseStruct->pt;

        switch (wParam) {
        case WM_MOUSEMOVE:
            emit instance->mouseMove(pt);
            break;
        case WM_RBUTTONDOWN:
            emit instance->buttonRDown(pt);
            return 1;
        case WM_RBUTTONUP:
            emit instance->buttonRUp(pt);
            return 1;
        case WM_LBUTTONDOWN:
            emit instance->buttonLDown(pt);
            return 1;
        case WM_LBUTTONUP:
            emit instance->buttonLUp(pt);
            return 1;
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
