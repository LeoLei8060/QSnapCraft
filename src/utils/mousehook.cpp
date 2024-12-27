#include "mousehook.h"
#include "../hooks/hooks.h"
#include <QDebug>

MouseHook *MouseHook::instance = nullptr;

MouseHook::MouseHook(QObject *parent)
    : QObject(parent)
    , m_mouseHook(NULL)
{
    instance = this;
}

MouseHook::~MouseHook()
{
    uninstall();
}

void MouseHook::install()
{
    if (!m_mouseHook) {
        m_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, GetModuleHandle(NULL), 0);
        if (m_mouseHook) {
            qDebug() << "Mouse hook installed successfully";
        }
    }

    // 调用hooks DLL中的接口，因为全局的WH_CALLWNDPROC钩子必须封装在单独的DLL中
    if (!InstallHooks()) {
        DWORD error = GetLastError();
        qDebug() << "Global hook installation failed with error:" << error;
    } else {
        qDebug() << "Global hook installed successfully";
    }
}

void MouseHook::uninstall()
{
    if (m_mouseHook) {
        UnhookWindowsHookEx(m_mouseHook);
        m_mouseHook = NULL;
    }
    UninstallHooks();
}

LRESULT CALLBACK MouseHook::mouseProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0 && instance) {
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
    return CallNextHookEx(NULL, code, wParam, lParam);
}
