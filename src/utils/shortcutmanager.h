#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <windows.h>
#include <QAbstractNativeEventFilter>
#include <QMap>
#include <QObject>

class ShortcutManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    enum EKeyType { captureKey = 1, captureCopyKey, showLastPinKey, showPinsKey, EscapeKey };

    static ShortcutManager *instance();

    // 注册快捷键
    bool registerHotkey(const QString &shortcut, int id);
    // 注销快捷键
    bool unregisterHotkey(int id);
    // 注销所有快捷键
    void unregisterAllHotkeys();

signals:
    // 当快捷键被触发时发出信号
    void hotkeyTriggered(int id);

public: // QAbstractNativeEventFilter interface
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

private:
    ShortcutManager(QObject *parent = nullptr);
    ~ShortcutManager();

    // 删除拷贝构造函数和赋值运算符
    ShortcutManager(const ShortcutManager &) = delete;
    ShortcutManager &operator=(const ShortcutManager &) = delete;

    // 解析快捷键字符串
    static QStringList parseShortcutString(const QString &shortcut);
    // 获取按键的虚拟键码
    static UINT getVirtualKeyCode(const QString &keyStr);
    // 获取修饰键的标志
    static UINT getModifiers(const QStringList &keys);

private:
    static ShortcutManager      *s_instance;
    QMap<int, QPair<UINT, UINT>> m_registeredHotkeys; // id -> {vk, modifiers}
};

#endif // SHORTCUTMANAGER_H
