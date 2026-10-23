#include "pinboard/pinwindow.h"
#include "pinboard/pinpool.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QGraphicsDropShadowEffect>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QPixmap>
#include <QPointer>
#include <QScreen>
#include <QTransform>
#include <QWheelEvent>
#include <QtTest>

class PinWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void pinImageKeepsContentAtRequestedPosition();
    void contextMenuContainsRequestedActions();
    void rightClickShowsContextMenu();
    void shadowActionTogglesWindowShadow();
    void copyAndTransformActionsUpdateImage();
    void closeHidesAndDestroyDeletesWindow();
    void clipboardImageCreatesPinnedWindow();
    void clipboardTextCreatesPinnedWindow();
    void arrowKeysMovePinnedWindow();
    void wheelZoomsAroundContentCenter();
    void keyboardPlusMinusZoom();
    void controlWheelAdjustsOpacityWithinLimits();
    void recoverLastPinShowsClosedWindow();
    void togglePinsVisibilityHidesAndShowsAll();
    void clickThroughTogglesPinUnderPoint();
    void offscreenPinIsClampedAfterTopologyChange();
    void appliesShadowAndStayOnTopSettings();
};

namespace {
QAction *action(PinWindow &window, const QString &objectName)
{
    auto *context_menu = window.findChild<QMenu *>(QStringLiteral("menu_PinWindow"));
    return context_menu != nullptr ? context_menu->findChild<QAction *>(objectName) : nullptr;
}

QImage copiedImage(PinWindow &window)
{
    QApplication::clipboard()->clear();
    QAction *copy_action = action(window, QStringLiteral("action_CopyImage"));
    if (copy_action != nullptr) {
        copy_action->trigger();
    }
    return QApplication::clipboard()->image();
}

QPixmap testImage()
{
    QImage image(4, 2, QImage::Format_ARGB32);
    image.fill(QColor(Qt::red));
    image.setPixelColor(0, 0, QColor(Qt::green));
    image.setPixelColor(3, 1, QColor(Qt::blue));
    return QPixmap::fromImage(image);
}
} // namespace

void PinWindowTest::pinImageKeepsContentAtRequestedPosition()
{
    PinWindow   window;
    const QRect content_rect(QPoint(140, 160), QSize(40, 30));
    window.pinImage(QPixmap(content_rect.size()), content_rect);

    auto *content = window.findChild<QLabel *>(QStringLiteral("pinImageContent"));
    QVERIFY(content);
    QCOMPARE(content->mapToGlobal(QPoint()), content_rect.topLeft());
    QCOMPARE(content->size(), content_rect.size());
}

void PinWindowTest::contextMenuContainsRequestedActions()
{
    PinWindow window;
    auto     *menu = window.findChild<QMenu *>(QStringLiteral("menu_PinWindow"));
    QVERIFY(menu);

    const QStringList expected_actions = {
        QStringLiteral("action_CopyImage"),
        QStringLiteral("action_SaveImageAs"),
        QStringLiteral("action_WindowShadow"),
    };
    for (const QString &object_name : expected_actions) {
        QVERIFY2(menu->findChild<QAction *>(object_name), qPrintable(object_name));
    }

    auto *processing_menu = menu->findChild<QMenu *>(QStringLiteral("menu_ImageProcessing"));
    QVERIFY(processing_menu);
    const QStringList transform_actions = {
        QStringLiteral("action_RotateLeft"),
        QStringLiteral("action_RotateRight"),
        QStringLiteral("action_FlipHorizontal"),
        QStringLiteral("action_FlipVertical"),
    };
    for (const QString &object_name : transform_actions) {
        QVERIFY2(processing_menu->findChild<QAction *>(object_name), qPrintable(object_name));
    }

    QVERIFY(menu->findChild<QAction *>(QStringLiteral("action_ClosePin")));
    QVERIFY(menu->findChild<QAction *>(QStringLiteral("action_DestroyPin")));
}

void PinWindowTest::rightClickShowsContextMenu()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(200, 200), testImage().size()));
    window.show();
    QCoreApplication::processEvents();

    auto *menu = window.findChild<QMenu *>(QStringLiteral("menu_PinWindow"));
    QVERIFY(menu);
    const QPoint      local_position = window.rect().center();
    QContextMenuEvent context_event(QContextMenuEvent::Mouse, local_position, window.mapToGlobal(local_position));
    QCoreApplication::sendEvent(&window, &context_event);

    QTRY_VERIFY_WITH_TIMEOUT(menu->isVisible(), 1000);
    menu->close();
}

void PinWindowTest::shadowActionTogglesWindowShadow()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(200, 200), testImage().size()));

    auto    *effect = window.findChild<QGraphicsDropShadowEffect *>();
    QAction *shadow_action = action(window, QStringLiteral("action_WindowShadow"));
    QVERIFY(effect);
    QVERIFY(shadow_action);
    QVERIFY(shadow_action->isCheckable());
    QVERIFY(shadow_action->isChecked());
    QVERIFY(effect->isEnabled());

    shadow_action->trigger();
    QVERIFY(!shadow_action->isChecked());
    QVERIFY(!effect->isEnabled());

    shadow_action->trigger();
    QVERIFY(shadow_action->isChecked());
    QVERIFY(effect->isEnabled());
}

void PinWindowTest::copyAndTransformActionsUpdateImage()
{
    PinWindow     window;
    const QPixmap source = testImage();
    window.pinImage(source, QRect(QPoint(200, 200), source.size()));

    QCOMPARE(copiedImage(window), source.toImage());

    QAction *rotate_left = action(window, QStringLiteral("action_RotateLeft"));
    QAction *rotate_right = action(window, QStringLiteral("action_RotateRight"));
    QVERIFY(rotate_left);
    QVERIFY(rotate_right);

    rotate_left->trigger();
    const QImage rotated_left = source.toImage().transformed(QTransform().rotate(-90));
    QCOMPARE(copiedImage(window), rotated_left);

    rotate_right->trigger();
    QCOMPARE(copiedImage(window), source.toImage());

    QAction *flip_horizontal = action(window, QStringLiteral("action_FlipHorizontal"));
    QAction *flip_vertical = action(window, QStringLiteral("action_FlipVertical"));
    QVERIFY(flip_horizontal);
    QVERIFY(flip_vertical);

    flip_horizontal->trigger();
    QCOMPARE(copiedImage(window), source.toImage().mirrored(true, false));
    flip_vertical->trigger();
    QCOMPARE(copiedImage(window), source.toImage().mirrored(true, true));
}

void PinWindowTest::closeHidesAndDestroyDeletesWindow()
{
    auto               *window = new PinWindow();
    QPointer<PinWindow> guard(window);
    window->pinImage(testImage(), QRect(QPoint(200, 200), testImage().size()));
    window->show();

    QAction *close_action = action(*window, QStringLiteral("action_ClosePin"));
    QVERIFY(close_action);
    close_action->trigger();
    QVERIFY(!window->isVisible());
    QVERIFY(!guard.isNull());

    QAction *destroy_action = action(*window, QStringLiteral("action_DestroyPin"));
    QVERIFY(destroy_action);
    destroy_action->trigger();
    QTRY_VERIFY_WITH_TIMEOUT(guard.isNull(), 1000);
}

void PinWindowTest::clipboardImageCreatesPinnedWindow()
{
    const QImage clipboard_image = testImage().toImage();
    QApplication::clipboard()->setImage(clipboard_image);
    QTest::qWait(20);
    if (QApplication::clipboard()->image().isNull()) {
        QSKIP("System clipboard is unavailable in this test session");
    }

    PinPool manager;
    QVERIFY(QMetaObject::invokeMethod(&manager, "pasteFromClipboard"));

    PinWindow *pinned_window = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *pin_window = qobject_cast<PinWindow *>(widget); pin_window != nullptr && pin_window->isVisible()) {
            pinned_window = pin_window;
            break;
        }
    }
    QVERIFY(pinned_window);

    auto *content = pinned_window->findChild<QLabel *>(QStringLiteral("pinImageContent"));
    QVERIFY(content);
    QCOMPARE(content->pixmap().toImage(), clipboard_image);
}

void PinWindowTest::clipboardTextCreatesPinnedWindow()
{
    QApplication::clipboard()->setText(QStringLiteral("QSnapCraft\nText to image"));
    QTest::qWait(20);
    if (QApplication::clipboard()->text().isEmpty()) {
        QSKIP("System clipboard is unavailable in this test session");
    }

    PinPool manager;
    QVERIFY(QMetaObject::invokeMethod(&manager, "pasteFromClipboard"));

    PinWindow *pinned_window = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *pin_window = qobject_cast<PinWindow *>(widget); pin_window != nullptr && pin_window->isVisible()) {
            pinned_window = pin_window;
            break;
        }
    }
    QVERIFY(pinned_window);

    auto *content = pinned_window->findChild<QLabel *>(QStringLiteral("pinImageContent"));
    QVERIFY(content);
    QVERIFY(!content->pixmap().isNull());
    QVERIFY(content->pixmap().width() > 40);
    QVERIFY(content->pixmap().height() > 30);
}

void PinWindowTest::arrowKeysMovePinnedWindow()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();
    window.setFocus();

    const QPoint start = window.pos();
    QTest::keyClick(&window, Qt::Key_Left);
    QTest::keyClick(&window, Qt::Key_Up);
    QCOMPARE(window.pos(), start + QPoint(-1, -1));

    QTest::keyClick(&window, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(window.pos(), start + QPoint(9, -1));
}

void PinWindowTest::wheelZoomsAroundContentCenter()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();

    auto *content = window.findChild<QLabel *>(QStringLiteral("pinImageContent"));
    QVERIFY(content);
    const QPoint center_before = content->mapToGlobal(content->rect().center());
    const QSize  size_before = content->size();

    const QPoint local_position = window.rect().center();
    const QPoint global_position = window.mapToGlobal(local_position);
    QWheelEvent  zoom_event(local_position,
                           global_position,
                           QPoint(),
                           QPoint(0, 120),
                           Qt::NoButton,
                           Qt::NoModifier,
                           Qt::NoScrollPhase,
                           false);
    QCoreApplication::sendEvent(&window, &zoom_event);

    QVERIFY(content->width() > size_before.width());
    QVERIFY(content->height() > size_before.height());
    QVERIFY((content->mapToGlobal(content->rect().center()) - center_before).manhattanLength() <= 2);
}

void PinWindowTest::keyboardPlusMinusZoom()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();
    window.setFocus();

    auto *content = window.findChild<QLabel *>(QStringLiteral("pinImageContent"));
    QVERIFY(content);
    const QSize initial_size = content->size();

    QTest::keyClick(&window, Qt::Key_Plus);
    QVERIFY(content->width() > initial_size.width());
    const QSize zoomed_size = content->size();

    QTest::keyClick(&window, Qt::Key_Minus);
    QVERIFY(content->width() < zoomed_size.width());
}

void PinWindowTest::controlWheelAdjustsOpacityWithinLimits()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();

    const QPoint local_position = window.rect().center();
    const QPoint global_position = window.mapToGlobal(local_position);
    for (int count = 0; count < 30; ++count) {
        QWheelEvent opacity_event(local_position,
                                  global_position,
                                  QPoint(),
                                  QPoint(0, -120),
                                  Qt::NoButton,
                                  Qt::ControlModifier,
                                  Qt::NoScrollPhase,
                                  false);
        QCoreApplication::sendEvent(&window, &opacity_event);
    }

    QVERIFY(window.windowOpacity() >= 0.19);
    QVERIFY(window.windowOpacity() <= 0.21);
}

void PinWindowTest::recoverLastPinShowsClosedWindow()
{
    PinPool manager;
    manager.pinImage(testImage(), QPoint(320, 320));

    PinWindow *pin_window = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            pin_window = candidate;
            break;
        }
    }
    QVERIFY(pin_window);

    QAction *close_action = action(*pin_window, QStringLiteral("action_ClosePin"));
    QVERIFY(close_action);
    close_action->trigger();
    QVERIFY(!pin_window->isVisible());

    bool recovered = false;
    QVERIFY(QMetaObject::invokeMethod(&manager, "recoverLastPin", Qt::DirectConnection, Q_RETURN_ARG(bool, recovered)));
    QVERIFY(recovered);
    QVERIFY(pin_window->isVisible());
}

void PinWindowTest::togglePinsVisibilityHidesAndShowsAll()
{
    PinPool manager;
    manager.pinImage(testImage(), QPoint(320, 320));
    manager.pinImage(testImage(), QPoint(500, 320));

    QVector<PinWindow *> pin_windows;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *pin_window = qobject_cast<PinWindow *>(widget); pin_window != nullptr && pin_window->isVisible()) {
            pin_windows.append(pin_window);
        }
    }
    QCOMPARE(pin_windows.size(), 2);

    QVERIFY(QMetaObject::invokeMethod(&manager, "togglePinsVisibility"));
    for (PinWindow *pin_window : pin_windows) {
        QVERIFY(!pin_window->isVisible());
    }

    QVERIFY(QMetaObject::invokeMethod(&manager, "togglePinsVisibility"));
    for (PinWindow *pin_window : pin_windows) {
        QVERIFY(pin_window->isVisible());
    }
}

void PinWindowTest::clickThroughTogglesPinUnderPoint()
{
    PinPool manager;
    manager.pinImage(testImage(), QPoint(320, 320));

    PinWindow *pin_window = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            pin_window = candidate;
            break;
        }
    }
    QVERIFY(pin_window);

    bool         toggled = false;
    const QPoint target = pin_window->geometry().center();
    QVERIFY(QMetaObject::invokeMethod(&manager,
                                      "toggleClickThroughAt",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(bool, toggled),
                                      Q_ARG(QPoint, target)));
    QVERIFY(toggled);
    QVERIFY(pin_window->testAttribute(Qt::WA_TransparentForMouseEvents));

    QVERIFY(QMetaObject::invokeMethod(&manager,
                                      "toggleClickThroughAt",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(bool, toggled),
                                      Q_ARG(QPoint, target)));
    QVERIFY(toggled);
    QVERIFY(!pin_window->testAttribute(Qt::WA_TransparentForMouseEvents));
}

void PinWindowTest::offscreenPinIsClampedAfterTopologyChange()
{
    PinPool manager;
    manager.pinImage(testImage(), QPoint(-10000, -10000));

    PinWindow *pin_window = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr) {
            pin_window = candidate;
            break;
        }
    }
    QVERIFY(pin_window);

    QVERIFY(QMetaObject::invokeMethod(&manager, "ensurePinsVisible"));
    QScreen *primary = QGuiApplication::primaryScreen();
    QVERIFY(primary);
    QVERIFY(primary->availableGeometry().contains(pin_window->frameGeometry()));
}

void PinWindowTest::appliesShadowAndStayOnTopSettings()
{
    PinWindow window;
    window.setShadowEnabled(false);
    window.setStayOnTop(false);
    window.pinImage(testImage(), QRect(QPoint(200, 200), testImage().size()));

    auto *effect = window.findChild<QGraphicsDropShadowEffect *>();
    QVERIFY(effect);
    QVERIFY(!effect->isEnabled());
    QVERIFY(!window.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
}

QTEST_MAIN(PinWindowTest)

#include "pinwindowtest.moc"
