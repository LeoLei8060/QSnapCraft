#include "pinboard/pinwindow.h"
#include "pinboard/pinpool.h"
#include "pinboard/pinsnapoverlay.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QGraphicsDropShadowEffect>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPixmap>
#include <QPointer>
#include <QScreen>
#include <QTemporaryDir>
#include <QTransform>
#include <QUrl>
#include <QWheelEvent>
#include <QtTest>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

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
    void highDpiClipboardImageKeepsLogicalSize();
    void explicitLogicalSizeKeepsHighResolutionCapture();
    void arrowKeysMovePinnedWindow();
    void arrowKeysReportUpdatedContentGeometry();
    void sizeChangeReportsContentGeometryOnce();
    void wheelZoomsAroundContentCenter();
    void keyboardPlusMinusZoom();
    void keyboardRotationAndFlip();
    void rotationPreservesDisplayedSize();
    void controlNumberKeysSetOpacityPresets();
    void controlWheelAdjustsOpacityWithinLimits();
    void wheelZoomsAroundCursorByDefault();
    void draggingSnapsToConfiguredTarget();
    void hiddenPinsDoNotParticipateInMagneticSnapping();
    void smoothScalingCanBeDisabled();
    void dropReplacesPinnedImage();
    void recoverLastPinShowsClosedWindow();
    void recoveryPoolDropsOldestPinBeyondLimit();
    void togglePinsVisibilityHidesAndShowsAll();
    void showAllDoesNotReopenClosedPins();
    void clickThroughTogglesPinUnderPoint();
    void clickThroughUsesTopmostOverlappingPin();
    void clickThroughSurvivesStayOnTopChange();
    void offscreenPinIsClampedAfterTopologyChange();
    void appliesShadowAndStayOnTopSettings();
    void windowShadowOverrideSurvivesUnrelatedSettingsRefresh();
    void persistsStablePinAndGroupState();
    void groupOperationsManageAllMembers();
    void gifClipboardCreatesPlayingPin();
    void persistsAnimatedPin();
    void animationMenuProvidesFrameAndSpeedControls();
    void snapOverlayUsesGuideBounds();
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

void PinWindowTest::highDpiClipboardImageKeepsLogicalSize()
{
    QImage image(200, 120, QImage::Format_ARGB32);
    image.fill(Qt::red);
    QPixmap pixmap = QPixmap::fromImage(image);
    pixmap.setDevicePixelRatio(2.0);
    QApplication::clipboard()->setPixmap(pixmap);
    QTest::qWait(20);
    if (QApplication::clipboard()->pixmap().isNull()) {
        QSKIP("System clipboard is unavailable in this test session");
    }

    PinPool manager;
    QVERIFY(QMetaObject::invokeMethod(&manager, "pasteFromClipboard"));

    PinWindow *pinnedWindow = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            pinnedWindow = candidate;
            break;
        }
    }
    QVERIFY(pinnedWindow);
    QCOMPARE(pinnedWindow->contentRect().size(), QSize(100, 60));
}

void PinWindowTest::explicitLogicalSizeKeepsHighResolutionCapture()
{
    QImage image(200, 120, QImage::Format_ARGB32);
    image.fill(Qt::red);

    PinPool manager;
    manager.pinImage(QPixmap::fromImage(image), QPoint(320, 320), QSize(100, 60));

    PinWindow *pinnedWindow = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            pinnedWindow = candidate;
            break;
        }
    }
    QVERIFY(pinnedWindow);
    QCOMPARE(pinnedWindow->contentRect().size(), QSize(100, 60));
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

void PinWindowTest::arrowKeysReportUpdatedContentGeometry()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();
    window.setFocus();

    QSignalSpy geometrySpy(&window, &PinWindow::sigContentGeometryChanged);
    QTest::keyClick(&window, Qt::Key_Right);
    QCOMPARE(geometrySpy.count(), 1);
    const QRect reported = geometrySpy.constFirst().constLast().value<QRect>();
    QCOMPARE(reported.left(), 301);
}

void PinWindowTest::sizeChangeReportsContentGeometryOnce()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();
    window.setFocus();

    QSignalSpy geometrySpy(&window, &PinWindow::sigContentGeometryChanged);
    QTest::keyClick(&window, Qt::Key_Plus);
    QCOMPARE(geometrySpy.count(), 1);
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

void PinWindowTest::keyboardRotationAndFlip()
{
    PinWindow     window;
    const QPixmap source = testImage();
    window.pinImage(source, QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();
    window.setFocus();

    QTest::keyClick(&window, Qt::Key_1);
    QCOMPARE(copiedImage(window), source.toImage().transformed(QTransform().rotate(-90)));

    QTest::keyClick(&window, Qt::Key_2);
    QCOMPARE(copiedImage(window), source.toImage());

    QTest::keyClick(&window, Qt::Key_3);
    QCOMPARE(copiedImage(window), source.toImage().mirrored(true, false));

    QTest::keyClick(&window, Qt::Key_4);
    QCOMPARE(copiedImage(window), source.toImage().mirrored(true, true));
}

void PinWindowTest::rotationPreservesDisplayedSize()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();
    window.setFocus();

    QTest::keyClick(&window, Qt::Key_1);
    QCOMPARE(window.contentRect().size(), QSize(80, 100));
}

void PinWindowTest::controlNumberKeysSetOpacityPresets()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();
    window.setFocus();

    QTest::keyClick(&window, Qt::Key_4, Qt::ControlModifier);
    QCOMPARE(window.windowOpacity(), 0.4);

    QTest::keyClick(&window, Qt::Key_0, Qt::ControlModifier);
    QCOMPARE(window.windowOpacity(), 1.0);
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

void PinWindowTest::wheelZoomsAroundCursorByDefault()
{
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();

    auto *content = window.findChild<QLabel *>(QStringLiteral("pinImageContent"));
    QVERIFY(content);
    const QPoint anchorBefore = content->mapToGlobal(QPoint(content->width() / 4, content->height() / 2));
    const QPoint localPosition = window.mapFromGlobal(anchorBefore);

    QWheelEvent zoomEvent(localPosition,
                          anchorBefore,
                          QPoint(),
                          QPoint(0, 120),
                          Qt::NoButton,
                          Qt::NoModifier,
                          Qt::NoScrollPhase,
                          false);
    QCoreApplication::sendEvent(&window, &zoomEvent);

    const QPoint anchorAfter = content->mapToGlobal(QPoint(content->width() / 4, content->height() / 2));
    QVERIFY((anchorAfter - anchorBefore).manhattanLength() <= 2);
}

void PinWindowTest::draggingSnapsToConfiguredTarget()
{
    PinWindow window;
    window.setSnapDistancePx(8);
    window.setSnapTargets({QRect(300, 330, 100, 80)});
    window.pinImage(testImage(), QRect(QPoint(194, 300), QSize(100, 80)));
    window.show();

    QSignalSpy   guideSpy(&window, &PinWindow::sigSnapGuidesChanged);
    const QPoint localPosition = window.rect().center();
    const QPoint globalPosition = window.mapToGlobal(localPosition);
    QMouseEvent  press(QEvent::MouseButtonPress,
                      localPosition,
                      globalPosition,
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &press);

    QMouseEvent move(QEvent::MouseMove,
                     localPosition + QPoint(2, 0),
                     globalPosition + QPoint(2, 0),
                     Qt::NoButton,
                     Qt::LeftButton,
                     Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &move);

    QCOMPARE(window.contentRect().left(), 200);
    QTRY_VERIFY(!guideSpy.isEmpty());
    const QVector<QLineF> guides = guideSpy.constLast().constLast().value<QVector<QLineF>>();
    QCOMPARE(guides.size(), 1);
    QCOMPARE(guides.constFirst().x1(), 300.0);

    QMouseEvent release(QEvent::MouseButtonRelease,
                        localPosition + QPoint(2, 0),
                        globalPosition + QPoint(2, 0),
                        Qt::LeftButton,
                        Qt::NoButton,
                        Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &release);
}

void PinWindowTest::hiddenPinsDoNotParticipateInMagneticSnapping()
{
    PinPool manager;
    manager.pinImage(testImage().scaled(100, 80), QPoint(300, 330));

    PinWindow *targetPin = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            targetPin = candidate;
            break;
        }
    }
    QVERIFY(targetPin);

    manager.pinImage(testImage().scaled(100, 80), QPoint(194, 300));
    PinWindow *movingPin = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget);
            candidate != nullptr && candidate != targetPin && candidate->isVisible()) {
            movingPin = candidate;
            break;
        }
    }
    QVERIFY(movingPin);
    manager.hideAllPins();
    movingPin->show();

    const QPoint localPosition = movingPin->rect().center();
    const QPoint globalPosition = movingPin->mapToGlobal(localPosition);
    QMouseEvent  press(QEvent::MouseButtonPress,
                      localPosition,
                      globalPosition,
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(movingPin, &press);
    QMouseEvent move(QEvent::MouseMove,
                     localPosition + QPoint(2, 0),
                     globalPosition + QPoint(2, 0),
                     Qt::NoButton,
                     Qt::LeftButton,
                     Qt::NoModifier);
    QCoreApplication::sendEvent(movingPin, &move);

    QCOMPARE(movingPin->contentRect().left(), 196);
    QMouseEvent release(QEvent::MouseButtonRelease,
                        localPosition + QPoint(2, 0),
                        globalPosition + QPoint(2, 0),
                        Qt::LeftButton,
                        Qt::NoButton,
                        Qt::NoModifier);
    QCoreApplication::sendEvent(movingPin, &release);
}

void PinWindowTest::smoothScalingCanBeDisabled()
{
    QImage source(2, 2, QImage::Format_ARGB32);
    source.fill(Qt::black);
    source.setPixelColor(0, 0, Qt::red);
    source.setPixelColor(1, 1, Qt::white);
    const QPixmap pixmap = QPixmap::fromImage(source);

    PinWindow window;
    window.setSmoothScaling(false);
    window.pinImage(pixmap, QRect(QPoint(300, 300), QSize(80, 60)));

    auto *content = window.findChild<QLabel *>(QStringLiteral("pinImageContent"));
    QVERIFY(content);
    const QImage expected = source.scaled(content->size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QCOMPARE(content->pixmap().toImage().convertToFormat(QImage::Format_ARGB32),
             expected.convertToFormat(QImage::Format_ARGB32));
}

void PinWindowTest::dropReplacesPinnedImage()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("replacement.png"));
    QImage        replacement(5, 3, QImage::Format_ARGB32);
    replacement.fill(Qt::darkGreen);
    QVERIFY(replacement.save(path));

    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();

    QMimeData mimeData;
    mimeData.setUrls({QUrl::fromLocalFile(path)});
    const QPoint    position = window.rect().center();
    QDragEnterEvent dragEnter(position, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &dragEnter);
    QVERIFY(dragEnter.isAccepted());

    QDropEvent dropEvent(position, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &dropEvent);
    QVERIFY(dropEvent.isAccepted());
    QCOMPARE(copiedImage(window).convertToFormat(QImage::Format_ARGB32),
             replacement.convertToFormat(QImage::Format_ARGB32));
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

void PinWindowTest::recoveryPoolDropsOldestPinBeyondLimit()
{
    PinPool manager;
    manager.setRecoveryLimit(1);

    manager.pinImage(testImage(), QPoint(320, 320));
    PinWindow *first = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            first = candidate;
            break;
        }
    }
    QVERIFY(first);
    QPointer<PinWindow> firstGuard(first);
    action(*first, QStringLiteral("action_ClosePin"))->trigger();

    manager.pinImage(testImage(), QPoint(500, 320));
    PinWindow *second = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            second = candidate;
            break;
        }
    }
    QVERIFY(second);
    action(*second, QStringLiteral("action_ClosePin"))->trigger();

    QTRY_VERIFY_WITH_TIMEOUT(firstGuard.isNull(), 1000);
    bool recovered = false;
    QVERIFY(QMetaObject::invokeMethod(&manager, "recoverLastPin", Qt::DirectConnection, Q_RETURN_ARG(bool, recovered)));
    QVERIFY(recovered);
    QVERIFY(second->isVisible());
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

void PinWindowTest::showAllDoesNotReopenClosedPins()
{
    PinPool manager;
    manager.pinImage(testImage(), QPoint(320, 320));
    manager.pinImage(testImage(), QPoint(500, 320));

    QVector<PinWindow *> pinWindows;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *pinWindow = qobject_cast<PinWindow *>(widget); pinWindow != nullptr && pinWindow->isVisible()) {
            pinWindows.append(pinWindow);
        }
    }
    QCOMPARE(pinWindows.size(), 2);

    PinWindow *closedPin = pinWindows.constFirst();
    PinWindow *openPin = pinWindows.constLast();
    action(*closedPin, QStringLiteral("action_ClosePin"))->trigger();
    QVERIFY(!closedPin->isVisible());

    QVERIFY(QMetaObject::invokeMethod(&manager, "hideAllPins"));
    QVERIFY(!openPin->isVisible());
    QVERIFY(QMetaObject::invokeMethod(&manager, "showAllPins"));
    QVERIFY(openPin->isVisible());
    QVERIFY(!closedPin->isVisible());

    bool recovered = false;
    QVERIFY(QMetaObject::invokeMethod(&manager, "recoverLastPin", Qt::DirectConnection, Q_RETURN_ARG(bool, recovered)));
    QVERIFY(recovered);
    QVERIFY(closedPin->isVisible());
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

void PinWindowTest::clickThroughUsesTopmostOverlappingPin()
{
#ifndef Q_OS_WIN
    QSKIP("Windows native Z-order test");
#else
    PinPool manager;
    manager.pinImage(testImage().scaled(100, 80), QPoint(320, 320));
    PinWindow *expectedTop = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *pinWindow = qobject_cast<PinWindow *>(widget); pinWindow != nullptr && pinWindow->isVisible()) {
            expectedTop = pinWindow;
            break;
        }
    }
    QVERIFY(expectedTop);

    manager.pinImage(testImage().scaled(100, 80), QPoint(320, 320));
    PinWindow *otherPin = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *pinWindow = qobject_cast<PinWindow *>(widget);
            pinWindow != nullptr && pinWindow != expectedTop && pinWindow->isVisible()) {
            otherPin = pinWindow;
            break;
        }
    }
    QVERIFY(otherPin);

    SetWindowPos(reinterpret_cast<HWND>(expectedTop->winId()),
                 HWND_TOP,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    QTest::qWait(30);

    bool toggled = false;
    QVERIFY(QMetaObject::invokeMethod(&manager,
                                      "toggleClickThroughAt",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(bool, toggled),
                                      Q_ARG(QPoint, expectedTop->contentRect().center())));
    QVERIFY(toggled);
    QVERIFY(expectedTop->isClickThrough());
    QVERIFY(!otherPin->isClickThrough());
#endif
}

void PinWindowTest::clickThroughSurvivesStayOnTopChange()
{
#ifndef Q_OS_WIN
    QSKIP("Windows native style test");
#else
    PinWindow window;
    window.pinImage(testImage(), QRect(QPoint(300, 300), QSize(100, 80)));
    window.show();
    window.setClickThrough(true);
    window.setStayOnTop(false);

    const HWND     windowHandle = reinterpret_cast<HWND>(window.winId());
    const LONG_PTR style = GetWindowLongPtrW(windowHandle, GWL_EXSTYLE);
    QVERIFY((style & WS_EX_TRANSPARENT) != 0);
    QVERIFY(window.testAttribute(Qt::WA_TransparentForMouseEvents));
#endif
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

void PinWindowTest::windowShadowOverrideSurvivesUnrelatedSettingsRefresh()
{
    PinPool manager;
    manager.pinImage(testImage(), QPoint(320, 320));

    PinWindow *pinWindow = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            pinWindow = candidate;
            break;
        }
    }
    QVERIFY(pinWindow);
    QAction *shadowAction = action(*pinWindow, QStringLiteral("action_WindowShadow"));
    auto    *effect = pinWindow->findChild<QGraphicsDropShadowEffect *>();
    QVERIFY(shadowAction);
    QVERIFY(effect);

    shadowAction->trigger();
    QVERIFY(!effect->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(&manager, "applyPinSettings"));
    QVERIFY(!effect->isEnabled());
}

void PinWindowTest::persistsStablePinAndGroupState()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString pinId;
    QString groupId;

    {
        PinPool manager(directory.path());
        manager.pinImage(testImage(), QPoint(320, 320), QSize(100, 60));
        PinWindow *pinWindow = nullptr;
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
                pinWindow = candidate;
                break;
            }
        }
        QVERIFY(pinWindow);
        pinId = pinWindow->pinId();
        groupId = manager.createGroup(QStringLiteral("参考图"));
        manager.assignPinGroup(pinWindow, groupId);
        pinWindow->setWindowOpacity(0.55);
    }

    QVERIFY(QFile::exists(QDir(directory.path()).filePath(QStringLiteral("pins.json"))));

    PinPool restored(directory.path());
    QCOMPARE(restored.pinCount(), 1);
    QCOMPARE(restored.groups().size(), 1);
    PinWindow *restoredPin = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            restoredPin = candidate;
            break;
        }
    }
    QVERIFY(restoredPin);
    QCOMPARE(restoredPin->pinId(), pinId);
    QCOMPARE(restoredPin->groupId(), groupId);
    QVERIFY(qAbs(restoredPin->opacity() - 0.55) < 0.01);
}

void PinWindowTest::groupOperationsManageAllMembers()
{
    PinPool manager;
    manager.pinImage(testImage(), QPoint(320, 320));
    manager.pinImage(testImage(), QPoint(500, 320));

    QVector<PinWindow *> pinWindows;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *pinWindow = qobject_cast<PinWindow *>(widget); pinWindow != nullptr && pinWindow->isVisible()) {
            pinWindows.append(pinWindow);
        }
    }
    QCOMPARE(pinWindows.size(), 2);

    const QString groupId = manager.createGroup(QStringLiteral("参考图"));
    QVERIFY(!groupId.isEmpty());
    for (PinWindow *pinWindow : pinWindows) {
        manager.assignPinGroup(pinWindow, groupId);
        QCOMPARE(pinWindow->groupId(), groupId);
    }

    manager.setGroupVisible(groupId, false);
    for (PinWindow *pinWindow : pinWindows) {
        QVERIFY(!pinWindow->isVisible());
    }
    manager.setGroupVisible(groupId, true);
    for (PinWindow *pinWindow : pinWindows) {
        QVERIFY(pinWindow->isVisible());
    }

    QVERIFY(manager.renameGroup(groupId, QStringLiteral("重要参考")));
    QCOMPARE(manager.groups().constFirst().name, QStringLiteral("重要参考"));

    manager.closeGroup(groupId);
    for (PinWindow *pinWindow : pinWindows) {
        QVERIFY(!pinWindow->isVisible());
    }
    bool recovered = false;
    QVERIFY(QMetaObject::invokeMethod(&manager, "recoverLastPin", Qt::DirectConnection, Q_RETURN_ARG(bool, recovered)));
    QVERIFY(recovered);
    QVERIFY(QMetaObject::invokeMethod(&manager, "recoverLastPin", Qt::DirectConnection, Q_RETURN_ARG(bool, recovered)));
    QVERIFY(recovered);

    manager.removeGroup(groupId);
    QVERIFY(manager.groups().isEmpty());
    for (PinWindow *pinWindow : pinWindows) {
        QVERIFY(pinWindow->groupId().isEmpty());
    }
}

void PinWindowTest::gifClipboardCreatesPlayingPin()
{
    auto *mimeData = new QMimeData();
    mimeData->setData(QStringLiteral("image/gif"),
                      QByteArray::fromBase64("R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw=="));
    QApplication::clipboard()->setMimeData(mimeData);
    QTest::qWait(20);

    PinPool manager;
    QVERIFY(QMetaObject::invokeMethod(&manager, "pasteFromClipboard"));
    PinWindow *pinWindow = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            pinWindow = candidate;
            break;
        }
    }
    QVERIFY(pinWindow);
    auto *content = pinWindow->findChild<QLabel *>(QStringLiteral("pinImageContent"));
    QVERIFY(content);
    QVERIFY(content->movie() != nullptr);
    QAction *toggle = action(*pinWindow, QStringLiteral("action_ToggleAnimation"));
    QVERIFY(toggle);
    QVERIFY(toggle->isEnabled());
}

void PinWindowTest::persistsAnimatedPin()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    {
        auto *mimeData = new QMimeData();
        mimeData->setData(QStringLiteral("image/gif"),
                          QByteArray::fromBase64("R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw=="));
        QApplication::clipboard()->setMimeData(mimeData);
        QTest::qWait(20);

        PinPool manager(directory.path());
        QVERIFY(QMetaObject::invokeMethod(&manager, "pasteFromClipboard"));
    }

    PinPool    restored(directory.path());
    PinWindow *pinWindow = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            pinWindow = candidate;
            break;
        }
    }
    QVERIFY(pinWindow);
    auto *content = pinWindow->findChild<QLabel *>(QStringLiteral("pinImageContent"));
    QVERIFY(content);
    QVERIFY(content->movie() != nullptr);
}

void PinWindowTest::animationMenuProvidesFrameAndSpeedControls()
{
    auto *mimeData = new QMimeData();
    mimeData->setData(QStringLiteral("image/gif"),
                      QByteArray::fromBase64("R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw=="));
    QApplication::clipboard()->setMimeData(mimeData);
    QTest::qWait(20);

    PinPool manager;
    QVERIFY(QMetaObject::invokeMethod(&manager, "pasteFromClipboard"));
    PinWindow *pinWindow = nullptr;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<PinWindow *>(widget); candidate != nullptr && candidate->isVisible()) {
            pinWindow = candidate;
            break;
        }
    }
    QVERIFY(pinWindow);
    auto *menu = pinWindow->findChild<QMenu *>(QStringLiteral("menu_Animation"));
    QVERIFY(menu);
    QVERIFY(!menu->isVisible());
    QVERIFY(menu->findChild<QAction *>(QStringLiteral("action_PreviousFrame")));
    QVERIFY(menu->findChild<QAction *>(QStringLiteral("action_NextFrame")));
    QVERIFY(menu->findChild<QAction *>(QStringLiteral("action_AnimationSpeed50")));
    QVERIFY(menu->findChild<QAction *>(QStringLiteral("action_AnimationSpeed200")));
}

void PinWindowTest::snapOverlayUsesGuideBounds()
{
    PinSnapOverlay overlay;
    overlay.setGuides({QLineF(300, 200, 300, 400)});
    QVERIFY(overlay.isVisible());
    QVERIFY(overlay.width() <= 8);
    QVERIFY(overlay.height() >= 200);
    overlay.hide();
}

QTEST_MAIN(PinWindowTest)

#include "pinwindowtest.moc"
