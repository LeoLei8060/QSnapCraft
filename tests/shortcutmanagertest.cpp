#include "platform/windows/shortcutmanager.h"

#include <QtTest>

class ShortcutManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void invalidShortcutReportsVisibleError();
    void successfulRegistrationClearsPreviousError();
};

void ShortcutManagerTest::invalidShortcutReportsVisibleError()
{
    ShortcutManager *manager = ShortcutManager::instance();
    manager->unregisterHotkey(100);

    QVERIFY(!manager->registerHotkey(QStringLiteral("NotARealKey"), 100));
    QVERIFY(manager->hotkeyError(100).contains(QStringLiteral("无效")));
    manager->unregisterAllHotkeys();
    QVERIFY(manager->hotkeyError(100).isEmpty());
}

void ShortcutManagerTest::successfulRegistrationClearsPreviousError()
{
    ShortcutManager *manager = ShortcutManager::instance();
    QVERIFY(!manager->registerHotkey(QStringLiteral("NotARealKey"), 101));
    QVERIFY(!manager->hotkeyError(101).isEmpty());

    const QString shortcut = QStringLiteral("Ctrl+Alt+Shift+F12");
    if (!manager->registerHotkey(shortcut, 101)) {
        QSKIP("Test hotkey is unavailable in the current desktop session");
    }
    QVERIFY(manager->hotkeyError(101).isEmpty());
    manager->unregisterHotkey(101);
}

QTEST_GUILESS_MAIN(ShortcutManagerTest)

#include "shortcutmanagertest.moc"
