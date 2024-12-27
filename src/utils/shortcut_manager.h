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

private:
    void registerHotKey();
    void unregisterHotKey();

    static const int HOTKEY_ID = 1;
};
