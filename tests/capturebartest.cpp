#include "ui/capturebar.h"
#include "app/applicationaction.h"

#include <QtTest>

#include <QCursor>
#include <QToolButton>

class CaptureBarTest : public QObject
{
    Q_OBJECT

private slots:
    void buildsSevenCaptureButtons();
    void emitsImplementedActions();
    void ocrActionIsEnabled();
    void autoHideKeepsARevealHotspot();
    void enteringHotspotRevealsCaptureBar();
    void suppressPreventsAutoHide();
    void buttonsUseIconsWithoutText();
    void clampsDraggedBarInsideScreen();
    void cursorReturnCancelsHideAnimation();
    void freeDragKeepsArrangementAndPosition();
    void restoresSavedFreePlacement();
    void revealsWithDockOrientation_data();
    void revealsWithDockOrientation();
};

void CaptureBarTest::buildsSevenCaptureButtons()
{
    CaptureBar                 captureBar;
    const QList<QToolButton *> buttons = captureBar.findChildren<QToolButton *>();
    QCOMPARE(buttons.size(), 7);

    for (const ApplicationAction action : captureBarActions()) {
        QVERIFY(captureBar.findChild<QToolButton *>(actionObjectName(action)));
    }
}

void CaptureBarTest::emitsImplementedActions()
{
    CaptureBar captureBar;
    QSignalSpy actionSpy(&captureBar, &CaptureBar::sigActionTriggered);

    QToolButton *snipButton = captureBar.findChild<QToolButton *>(actionObjectName(ApplicationAction::kSnip));
    QToolButton *preferencesButton = captureBar.findChild<QToolButton *>(
        actionObjectName(ApplicationAction::kPreferences));
    QToolButton *rulerButton = captureBar.findChild<QToolButton *>(actionObjectName(ApplicationAction::kScreenRuler));

    QVERIFY(snipButton);
    QVERIFY(preferencesButton);
    QVERIFY(rulerButton);
    QTest::mouseClick(snipButton, Qt::LeftButton);
    QTest::mouseClick(rulerButton, Qt::LeftButton);
    QTest::mouseClick(preferencesButton, Qt::LeftButton);

    QCOMPARE(actionSpy.count(), 3);
    QCOMPARE(actionSpy.at(0).at(0).value<ApplicationAction>(), ApplicationAction::kSnip);
    QCOMPARE(actionSpy.at(1).at(0).value<ApplicationAction>(), ApplicationAction::kScreenRuler);
    QCOMPARE(actionSpy.at(2).at(0).value<ApplicationAction>(), ApplicationAction::kPreferences);
}

void CaptureBarTest::ocrActionIsEnabled()
{
    CaptureBar   captureBar;
    QToolButton *ocrButton = captureBar.findChild<QToolButton *>(actionObjectName(ApplicationAction::kOcrSnip));

    QVERIFY(ocrButton);
    QVERIFY(ocrButton->isVisibleTo(&captureBar));
    QVERIFY(ocrButton->isEnabled());
    QVERIFY(!ocrButton->toolTip().contains(QStringLiteral("开发中")));
}

void CaptureBarTest::autoHideKeepsARevealHotspot()
{
    CaptureBar captureBar;
    captureBar.setDockEdge(DockEdge::Right);
    captureBar.setAutoHideEnabled(true);
    captureBar.setHideDelayMs(20);
    captureBar.setSlideDurationMs(20);
    captureBar.showAtDock(DockEdge::Right, 0.5);

    captureBar.scheduleAutoHide();
    QTest::qWait(300);

    QVERIFY(captureBar.isHiddenToHotspot());
    QWidget *hotspot = captureBar.findChild<QWidget *>(QStringLiteral("hotspot_CaptureBar"));
    QVERIFY(hotspot);
    QCOMPARE(hotspot->size(), QSize(4, 50));
    const QRect hotspot_global(hotspot->mapToGlobal(QPoint()), hotspot->size());
    QScreen    *hotspot_screen = QGuiApplication::screenAt(hotspot_global.center());
    QVERIFY(hotspot_screen);
    const QRect screen_rect = hotspot_screen->availableGeometry();
    QVERIFY2(screen_rect.contains(hotspot_global),
             qPrintable(QStringLiteral("screen=%1,%2 %3x%4 hotspot=%5,%6 %7x%8")
                            .arg(screen_rect.x())
                            .arg(screen_rect.y())
                            .arg(screen_rect.width())
                            .arg(screen_rect.height())
                            .arg(hotspot_global.x())
                            .arg(hotspot_global.y())
                            .arg(hotspot_global.width())
                            .arg(hotspot_global.height())));
    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (!render_directory.isEmpty()) {
        QVERIFY(hotspot->grab().save(render_directory + QStringLiteral("/capture-bar-hotspot.png")));
    }
}

void CaptureBarTest::enteringHotspotRevealsCaptureBar()
{
    CaptureBar captureBar;
    captureBar.setDockEdge(DockEdge::Right);
    captureBar.setAutoHideEnabled(true);
    captureBar.setHideDelayMs(20);
    captureBar.setSlideDurationMs(20);
    captureBar.setRevealDelayMs(20);
    captureBar.showAtDock(DockEdge::Right, 0.5);
    captureBar.scheduleAutoHide();
    QTest::qWait(300);

    QWidget *hotspot = captureBar.findChild<QWidget *>(QStringLiteral("hotspot_CaptureBar"));
    QVERIFY(hotspot);
    QVERIFY(hotspot->isVisible());

    const QPoint previous_cursor = QCursor::pos();
    QCursor::setPos(hotspot->mapToGlobal(hotspot->rect().center()));
    QEvent enterEvent(QEvent::Enter);
    QCoreApplication::sendEvent(hotspot, &enterEvent);
    QTest::qWait(150);

    QVERIFY(!captureBar.isHiddenToHotspot());
    QVERIFY(captureBar.isVisible());
    QScreen *bar_screen = QGuiApplication::screenAt(captureBar.geometry().center());
    QVERIFY(bar_screen);
    QVERIFY(bar_screen->availableGeometry().contains(captureBar.geometry()));
    QCursor::setPos(previous_cursor);
}

void CaptureBarTest::suppressPreventsAutoHide()
{
    CaptureBar captureBar;
    captureBar.setAutoHideEnabled(true);
    captureBar.suppressForCapture();
    captureBar.scheduleAutoHide();
    QTest::qWait(700);

    QVERIFY(!captureBar.isHiddenToHotspot());
}

void CaptureBarTest::buttonsUseIconsWithoutText()
{
    CaptureBar captureBar;
    for (const ApplicationAction action : captureBarActions()) {
        QToolButton *button = captureBar.findChild<QToolButton *>(actionObjectName(action));
        QVERIFY(button);
        QVERIFY(!button->icon().isNull());
        QVERIFY(button->text().isEmpty());
        QVERIFY(!button->toolTip().isEmpty());
    }

    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (!render_directory.isEmpty()) {
        const QPixmap image = captureBar.grab();
        QVERIFY(!image.isNull());
        QVERIFY(image.save(render_directory + QStringLiteral("/capture-bar-icons.png")));
    }
}

void CaptureBarTest::clampsDraggedBarInsideScreen()
{
    CaptureBar captureBar;
    captureBar.showAtDock(DockEdge::None, 0.5);
    QWidget *drag_handle = captureBar.findChild<QWidget *>(QStringLiteral("dragHandle_CaptureBar"));
    QVERIFY(drag_handle);

    const QRect  screen = captureBar.screen()->availableGeometry();
    const QPoint start = drag_handle->mapToGlobal(drag_handle->rect().center());
    const QPoint end(screen.right() + 80, screen.bottom() + 80);
    QMouseEvent  press(QEvent::MouseButtonPress,
                      QPointF(drag_handle->rect().center()),
                      QPointF(start),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(drag_handle, &press);
    QMouseEvent move(QEvent::MouseMove,
                     QPointF(drag_handle->rect().center()),
                     QPointF(end),
                     Qt::NoButton,
                     Qt::LeftButton,
                     Qt::NoModifier);
    QCoreApplication::sendEvent(drag_handle, &move);
    QMouseEvent release(QEvent::MouseButtonRelease,
                        QPointF(drag_handle->rect().center()),
                        QPointF(end),
                        Qt::LeftButton,
                        Qt::NoButton,
                        Qt::NoModifier);
    QCoreApplication::sendEvent(drag_handle, &release);

    QVERIFY(screen.contains(captureBar.geometry()));
}

void CaptureBarTest::cursorReturnCancelsHideAnimation()
{
    CaptureBar captureBar;
    captureBar.setDockEdge(DockEdge::Top);
    captureBar.setAutoHideEnabled(true);
    captureBar.setHideDelayMs(100);
    captureBar.setSlideDurationMs(300);
    captureBar.showAtDock(DockEdge::Top, 0.5);
    captureBar.scheduleAutoHide();
    QTest::qWait(150);

    QVERIFY(!captureBar.isHiddenToHotspot());
    const QPoint previous_cursor = QCursor::pos();
    QCursor::setPos(captureBar.mapToGlobal(captureBar.rect().center()));
    QEvent enter_event(QEvent::Enter);
    QCoreApplication::sendEvent(&captureBar, &enter_event);
    QTest::qWait(50);

    QVERIFY(captureBar.isVisible());
    QVERIFY(!captureBar.isHiddenToHotspot());
    QScreen *screen = QGuiApplication::screenAt(captureBar.geometry().center());
    QVERIFY(screen);
    QVERIFY(screen->availableGeometry().contains(captureBar.geometry()));
    QCursor::setPos(previous_cursor);
}

void CaptureBarTest::freeDragKeepsArrangementAndPosition()
{
    CaptureBar captureBar;
    captureBar.setAutoHideEnabled(false);
    captureBar.showAtDock(DockEdge::Top, 0.5);
    QVERIFY(captureBar.isHorizontalLayout());
    QSignalSpy placement_spy(&captureBar, &CaptureBar::sigPlacementChanged);

    QWidget *drag_handle = captureBar.findChild<QWidget *>(QStringLiteral("dragHandle_CaptureBar"));
    QVERIFY(drag_handle);
    const QRect  screen = captureBar.screen()->availableGeometry();
    const QPoint target(screen.left() + 320, screen.top() + 260);
    const QPoint drag_offset = drag_handle->mapToGlobal(drag_handle->rect().center()) - captureBar.pos();
    QMouseEvent  press(QEvent::MouseButtonPress,
                      QPointF(drag_handle->rect().center()),
                      QPointF(drag_handle->mapToGlobal(drag_handle->rect().center())),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(drag_handle, &press);
    QMouseEvent move(QEvent::MouseMove,
                     QPointF(drag_handle->rect().center()),
                     QPointF(target + drag_offset),
                     Qt::NoButton,
                     Qt::LeftButton,
                     Qt::NoModifier);
    QCoreApplication::sendEvent(drag_handle, &move);
    QMouseEvent release(QEvent::MouseButtonRelease,
                        QPointF(drag_handle->rect().center()),
                        QPointF(target + drag_offset),
                        Qt::LeftButton,
                        Qt::NoButton,
                        Qt::NoModifier);
    QCoreApplication::sendEvent(drag_handle, &release);

    QCOMPARE(static_cast<int>(captureBar.dockEdge()), static_cast<int>(DockEdge::None));
    QVERIFY(captureBar.isHorizontalLayout());
    QCOMPARE(captureBar.pos(), target);
    QCOMPARE(placement_spy.count(), 1);
    QCOMPARE(placement_spy.at(0).at(3).toPoint(), target);
    QVERIFY(placement_spy.at(0).at(4).toBool());
    QVERIFY(placement_spy.at(0).at(5).toBool());
}

void CaptureBarTest::restoresSavedFreePlacement()
{
    CaptureBar captureBar;
    captureBar.setAutoHideEnabled(false);
    captureBar.setFreeOrientation(true);
    const QRect  screen = captureBar.screen()->availableGeometry();
    const QPoint saved_position(screen.left() + 240, screen.top() + 180);
    captureBar.showAtDock(DockEdge::None, 0.5, {}, saved_position, true);

    QCOMPARE(captureBar.pos(), saved_position);
    QVERIFY(captureBar.isHorizontalLayout());
    QCOMPARE(static_cast<int>(captureBar.dockEdge()), static_cast<int>(DockEdge::None));
}

void CaptureBarTest::revealsWithDockOrientation_data()
{
    QTest::addColumn<int>("edge");
    QTest::addColumn<bool>("horizontal");
    QTest::newRow("top") << static_cast<int>(DockEdge::Top) << true;
    QTest::newRow("right") << static_cast<int>(DockEdge::Right) << false;
    QTest::newRow("left") << static_cast<int>(DockEdge::Left) << false;
}

void CaptureBarTest::revealsWithDockOrientation()
{
    QFETCH(int, edge);
    QFETCH(bool, horizontal);
    const auto dock_edge = static_cast<DockEdge>(edge);

    CaptureBar captureBar;
    captureBar.setAutoHideEnabled(true);
    captureBar.setHideDelayMs(100);
    captureBar.setSlideDurationMs(20);
    captureBar.setRevealDelayMs(20);
    captureBar.showAtDock(dock_edge, 0.5);
    QCOMPARE(captureBar.isHorizontalLayout(), horizontal);

    captureBar.scheduleAutoHide();
    QTest::qWait(300);
    QVERIFY(captureBar.isHiddenToHotspot());
    QWidget *hotspot = captureBar.findChild<QWidget *>(QStringLiteral("hotspot_CaptureBar"));
    QVERIFY(hotspot);

    const QPoint previous_cursor = QCursor::pos();
    QCursor::setPos(hotspot->mapToGlobal(hotspot->rect().center()));
    QEvent enter_event(QEvent::Enter);
    QCoreApplication::sendEvent(hotspot, &enter_event);
    QTest::qWait(150);

    QVERIFY(captureBar.isVisible());
    QVERIFY(!captureBar.isHiddenToHotspot());
    QCOMPARE(captureBar.isHorizontalLayout(), horizontal);
    QCOMPARE(static_cast<int>(captureBar.dockEdge()), edge);
    QCursor::setPos(previous_cursor);
}

QTEST_MAIN(CaptureBarTest)

#include "capturebartest.moc"
