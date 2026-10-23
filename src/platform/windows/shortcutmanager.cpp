#include "shortcutmanager.h"
#include <QApplication>
#include <QDebug>

// 初始化静态成员
ShortcutManager *ShortcutManager::s_instance = nullptr;

ShortcutManager *ShortcutManager::instance()
{
    if (!s_instance) {
        s_instance = new ShortcutManager(qApp);
    }
    return s_instance;
}

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent)
{
    // 安装事件过滤器
    qApp->installNativeEventFilter(this);
}

ShortcutManager::~ShortcutManager()
{
    // 注销所有快捷键
    unregisterAllHotkeys();
    qApp->removeNativeEventFilter(this);
}

bool ShortcutManager::registerHotkey(const QString &shortcut, int id)
{
    m_registrationErrors.remove(id);
    // 先注销已存在的快捷键
    unregisterHotkey(id);

    // 解析快捷键字符串
    QStringList keys = parseShortcutString(shortcut);
    if (keys.isEmpty()) {
        const QString error = QStringLiteral("快捷键无效：%1").arg(shortcut);
        m_registrationErrors[id] = error;
        qWarning() << error;
        emit sigRegistrationFailed(id, shortcut, error);
        return false;
    }

    // 获取修饰键和主键
    UINT modifiers = getModifiers(keys);
    UINT vk = getVirtualKeyCode(keys.last()); // 最后一个是主键
    if (vk == 0) {
        const QString error = QStringLiteral("快捷键无效，无法识别按键：%1").arg(shortcut);
        m_registrationErrors[id] = error;
        qWarning() << error;
        emit sigRegistrationFailed(id, shortcut, error);
        return false;
    }

    // 注册快捷键
    if (!RegisterHotKey(nullptr, id, modifiers, vk)) {
        const DWORD error_code = GetLastError();
        QString     error;
        switch (error_code) {
        case ERROR_HOTKEY_ALREADY_REGISTERED:
            error = QStringLiteral("快捷键已被其他程序占用：%1").arg(shortcut);
            break;
        case ERROR_INVALID_WINDOW_HANDLE:
            error = QStringLiteral("快捷键注册失败：无效的窗口句柄（%1）").arg(shortcut);
            break;
        case ERROR_INVALID_PARAMETER:
            error = QStringLiteral("快捷键参数无效：%1").arg(shortcut);
            break;
        default:
            error = QStringLiteral("快捷键注册失败：%1（错误码 %2）").arg(shortcut).arg(error_code);
        }
        m_registrationErrors[id] = error;
        qWarning() << error;
        emit sigRegistrationFailed(id, shortcut, error);
        return false;
    }

    // 保存注册的快捷键信息
    m_registeredHotkeys[id] = qMakePair(vk, modifiers);
    qDebug() << "Successfully registered hotkey:" << shortcut << "with id:" << id;
    return true;
}

bool ShortcutManager::unregisterHotkey(int id)
{
    m_registrationErrors.remove(id);
    if (!m_registeredHotkeys.contains(id)) {
        return true; // 快捷键不存在，视为成功
    }

    if (!UnregisterHotKey(nullptr, id)) {
        qWarning() << "Failed to unregister hotkey id:" << id << "Error:" << GetLastError();
        return false;
    }

    m_registeredHotkeys.remove(id);
    return true;
}

QString ShortcutManager::hotkeyError(int id) const
{
    return m_registrationErrors.value(id);
}

void ShortcutManager::unregisterAllHotkeys()
{
    QList<int> ids = m_registeredHotkeys.keys();
    for (int id : ids) {
        unregisterHotkey(id);
    }
    m_registrationErrors.clear();
}

bool ShortcutManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result)

    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY) {
            int  id = static_cast<int>(msg->wParam);
            emit hotkeyTriggered(id);
            return true;
        }
    }
    return false;
}

QStringList ShortcutManager::parseShortcutString(const QString &shortcut)
{
    // 按+号分割字符串，并去除每个部分的空白
    QStringList keys = shortcut.split('+', Qt::SkipEmptyParts);
    for (QString &key : keys) {
        key = key.trimmed();
    }
    return keys;
}

UINT ShortcutManager::getVirtualKeyCode(const QString &keyStr)
{
    // 特殊键映射
    static const QMap<QString, UINT> specialKeys = {{"Ctrl", VK_CONTROL},
                                                    {"Shift", VK_SHIFT},
                                                    {"Alt", VK_MENU},
                                                    {"Win", VK_LWIN},
                                                    {"Tab", VK_TAB},
                                                    {"Esc", VK_ESCAPE},
                                                    {"Space", VK_SPACE},
                                                    {"Enter", VK_RETURN},
                                                    {"Backspace", VK_BACK},
                                                    {"Insert", VK_INSERT},
                                                    {"Delete", VK_DELETE},
                                                    {"Home", VK_HOME},
                                                    {"End", VK_END},
                                                    {"PgUp", VK_PRIOR},
                                                    {"PgDown", VK_NEXT},
                                                    {"Left", VK_LEFT},
                                                    {"Right", VK_RIGHT},
                                                    {"Up", VK_UP},
                                                    {"Down", VK_DOWN}};

    // 检查是否是特殊键
    if (specialKeys.contains(keyStr)) {
        return specialKeys[keyStr];
    }

    // 检查功能键 (F1-F24)
    if (keyStr.startsWith('F')) {
        bool ok;
        int  num = keyStr.mid(1).toInt(&ok);
        if (ok && num >= 1 && num <= 24) {
            return VK_F1 + num - 1;
        }
    }

    // 检查数字键
    if (keyStr.length() == 1) {
        QChar ch = keyStr[0];
        if (ch.isDigit()) {
            return '0' + ch.digitValue();
        }
        // 检查字母键
        if (ch.isLetter()) {
            return ch.toUpper().toLatin1();
        }
    }

    // 无法识别的键
    return 0;
}

UINT ShortcutManager::getModifiers(const QStringList &keys)
{
    UINT modifiers = MOD_NOREPEAT; // 添加 MOD_NOREPEAT 标志防止按键重复

    // 遍历除最后一个键（主键）外的所有键
    for (int i = 0; i < keys.size() - 1; ++i) {
        const QString &key = keys[i];
        if (key == "Ctrl") {
            modifiers |= MOD_CONTROL;
        } else if (key == "Alt") {
            modifiers |= MOD_ALT;
        } else if (key == "Shift") {
            modifiers |= MOD_SHIFT;
        } else if (key == "Win") {
            modifiers |= MOD_WIN;
        }
    }

    return modifiers;
}

bool ShortcutManager::isHotkeyRegistered(const QString &shortcut, int id) const
{
    // 检查ID是否已注册
    if (!m_registeredHotkeys.contains(id)) {
        return false;
    }

    // 解析快捷键字符串
    QStringList keys = parseShortcutString(shortcut);
    if (keys.isEmpty()) {
        return false;
    }

    // 获取修饰键和主键
    UINT modifiers = getModifiers(keys);
    UINT vk = getVirtualKeyCode(keys.last()); // 最后一个是主键
    if (vk == 0) {
        return false;
    }

    // 获取已注册的快捷键信息
    const auto &registeredHotkey = m_registeredHotkeys[id];

    // 比较虚拟键码和修饰键是否匹配
    return registeredHotkey.first == vk && registeredHotkey.second == modifiers;
}
