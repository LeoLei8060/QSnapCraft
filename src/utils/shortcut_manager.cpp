#include "shortcut_manager.h"
#include <QApplication>
#include <QDebug>

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
    if (!RegisterHotKey(nullptr, HOTKEY_ID, 0, VK_F2)) {
        qDebug() << "Failed to register hotkey";
    }
}

void ShortcutManager::unregisterHotKey()
{
    UnregisterHotKey(nullptr, HOTKEY_ID);
}

bool ShortcutManager::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY) {
            if (msg->wParam == HOTKEY_ID) {
                emit screenshotTriggered();
                return true;
            }
        }
    }
    return false;
}
