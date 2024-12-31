#pragma once

#include <memory>
#include <windows.h>
#include <QAbstractNativeEventFilter>
#include <QKeySequence>
#include <QObject>

class ShortcutManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit ShortcutManager(QObject *parent = nullptr);
    ~ShortcutManager() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

signals:
    void screenshotTriggered();
    void escapePressed();

private:
    void registerHotKey();
    void unregisterHotKey();

    static const int SCREENSHOT_HOTKEY_ID = 1;
    static const int ESCAPE_HOTKEY_ID = 2;
};
