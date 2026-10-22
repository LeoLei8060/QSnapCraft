#include "app/applicationaction.h"

#include <QtTest>

class ApplicationActionTest : public QObject
{
    Q_OBJECT

private slots:
    void providesCaptureBarMetadata();
    void providesTrayActionMetadata();
    void marksCurrentActionsImplemented();
};

void ApplicationActionTest::providesCaptureBarMetadata()
{
    const QVector<ApplicationAction> actions = captureBarActions();

    QCOMPARE(actions.size(), 7);
    QCOMPARE(static_cast<int>(actions.at(0)), static_cast<int>(ApplicationAction::kSnip));
    QCOMPARE(static_cast<int>(actions.at(6)), static_cast<int>(ApplicationAction::kPreferences));
    QVERIFY(!actionDisplayName(ApplicationAction::kOcrSnip).isEmpty());
    QVERIFY(!actionObjectName(ApplicationAction::kOcrSnip).isEmpty());
}

void ApplicationActionTest::providesTrayActionMetadata()
{
    QCOMPARE(actionDisplayName(ApplicationAction::kToggleCaptureBar), QStringLiteral("显示桌面工具条"));
    QCOMPARE(actionDisplayName(ApplicationAction::kAbout), QStringLiteral("关于"));
    QCOMPARE(actionObjectName(ApplicationAction::kToggleCaptureBar),
             QStringLiteral("action_ToggleCaptureBar"));
}

void ApplicationActionTest::marksCurrentActionsImplemented()
{
    QVERIFY(isImplementedAction(ApplicationAction::kSnip));
    QVERIFY(isImplementedAction(ApplicationAction::kCaptureActiveWindow));
    QVERIFY(isImplementedAction(ApplicationAction::kPickColor));
    QVERIFY(isImplementedAction(ApplicationAction::kScreenRuler));
    QVERIFY(isImplementedAction(ApplicationAction::kPreferences));
    QVERIFY(isImplementedAction(ApplicationAction::kToggleCaptureBar));
    QVERIFY(isImplementedAction(ApplicationAction::kAbout));
    QVERIFY(isImplementedAction(ApplicationAction::kQuit));
    QVERIFY(!isImplementedAction(ApplicationAction::kOcrSnip));
    QVERIFY(!isImplementedAction(ApplicationAction::kScrollSnip));
}

QTEST_APPLESS_MAIN(ApplicationActionTest)

#include "applicationactiontest.moc"
