#include "app/applicationaction.h"
#include "ui/systemtray.h"

#include <QAction>
#include <QMenu>
#include <QtTest>

class SystemTrayTest : public QObject
{
    Q_OBJECT

private slots:
    void buildsRedesignedCaptureMenu();
    void emitsSharedApplicationActions();
};

void SystemTrayTest::buildsRedesignedCaptureMenu()
{
    SystemTray tray;
    QMenu *menu = tray.contextMenu();
    QVERIFY(menu);

    QVector<QAction *> actions;
    for (QAction *action : menu->actions()) {
        if (!action->isSeparator()) {
            actions.append(action);
        }
    }

    QCOMPARE(actions.size(), 10);
    QCOMPARE(actions.at(0)->data().value<ApplicationAction>(), ApplicationAction::kSnip);
    QCOMPARE(actions.at(1)->data().value<ApplicationAction>(), ApplicationAction::kCaptureActiveWindow);
    QCOMPARE(actions.at(2)->data().value<ApplicationAction>(), ApplicationAction::kScrollSnip);
    QCOMPARE(actions.at(3)->data().value<ApplicationAction>(), ApplicationAction::kOcrSnip);
    QCOMPARE(actions.at(4)->data().value<ApplicationAction>(), ApplicationAction::kPickColor);
    QCOMPARE(actions.at(5)->data().value<ApplicationAction>(), ApplicationAction::kScreenRuler);
    QCOMPARE(actions.at(6)->data().value<ApplicationAction>(), ApplicationAction::kToggleCaptureBar);
    QCOMPARE(actions.at(7)->data().value<ApplicationAction>(), ApplicationAction::kPreferences);
    QCOMPARE(actions.at(8)->data().value<ApplicationAction>(), ApplicationAction::kAbout);
    QCOMPARE(actions.at(9)->data().value<ApplicationAction>(), ApplicationAction::kQuit);

    QVERIFY(!actions.at(2)->isEnabled());
    QVERIFY(!actions.at(3)->isEnabled());
    QVERIFY(actions.at(2)->toolTip().contains(QStringLiteral("开发中")));
    QVERIFY(actions.at(3)->toolTip().contains(QStringLiteral("开发中")));
    QVERIFY(actions.at(6)->isCheckable());
    QVERIFY(actions.at(9)->isEnabled());
    QCOMPARE(actions.at(9)->objectName(), QStringLiteral("action_Quit"));
    for (QAction *action : actions) {
        QVERIFY(!action->toolTip().isEmpty());
        QVERIFY(!action->icon().isNull());
    }

    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (!render_directory.isEmpty()) {
        menu->popup(QPoint(200, 200));
        QTest::qWait(50);
        QVERIFY(menu->grab().save(render_directory + QStringLiteral("/tray-menu.png")));
        menu->hide();
    }
}

void SystemTrayTest::emitsSharedApplicationActions()
{
    SystemTray tray;
    QSignalSpy actionSpy(&tray, &SystemTray::sigActionTriggered);
    QMenu *menu = tray.contextMenu();

    QAction *preferences = nullptr;
    QAction *quit = nullptr;
    for (QAction *action : menu->actions()) {
        if (action->data().value<ApplicationAction>() == ApplicationAction::kPreferences) {
            preferences = action;
        } else if (action->data().value<ApplicationAction>() == ApplicationAction::kQuit) {
            quit = action;
        }
    }
    QVERIFY(preferences);
    QVERIFY(quit);
    preferences->trigger();
    quit->trigger();

    QCOMPARE(actionSpy.count(), 2);
    QCOMPARE(actionSpy.at(0).at(0).value<ApplicationAction>(), ApplicationAction::kPreferences);
    QCOMPARE(actionSpy.at(1).at(0).value<ApplicationAction>(), ApplicationAction::kQuit);
}

QTEST_MAIN(SystemTrayTest)

#include "systemtraytest.moc"
