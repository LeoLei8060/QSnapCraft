#include "shortcutmanager.h"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent)
{
    qApp->installNativeEventFilter(this);
    registerHotKey();
}

ShortcutManager::~ShortcutManager()
{
    unregisterHotKey();
}

void ShortcutManager::registerHotKey()
{
    if (!RegisterHotKey(nullptr, SCREENSHOT_HOTKEY_ID, 0, VK_F1)) {
        QMessageBox::warning(nullptr, "警告", "F1 快捷键注册失败！");
    }
    if (!RegisterHotKey(nullptr, ESCAPE_HOTKEY_ID, 0, VK_ESCAPE)) {
        QMessageBox::warning(nullptr, "警告", "ESC 快捷键注册失败！");
    }
}

void ShortcutManager::unregisterHotKey()
{
    UnregisterHotKey(nullptr, SCREENSHOT_HOTKEY_ID);
    UnregisterHotKey(nullptr, ESCAPE_HOTKEY_ID);
}

bool ShortcutManager::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY) {
            if (msg->wParam == SCREENSHOT_HOTKEY_ID) {
                emit screenshotTriggered();
                return true;
            } else if (msg->wParam == ESCAPE_HOTKEY_ID) {
                emit escapePressed();
                return true;
            }
        }
    }
    return false;
}
