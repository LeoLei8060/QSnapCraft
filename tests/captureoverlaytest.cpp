#include "ui/captureoverlay.h"
#include "capture/captureoptions.h"
#include "capture/capturetypes.h"
#include "output/outputservice.h"
#include "paint/paintitem.h"
#include "paint/paintscene.h"
#include "ui/paintbar.h"

#include <QtTest>

#include <QClipboard>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGuiApplication>
#include <QImage>
#include <QMouseEvent>
#include <QSlider>
#include <QToolButton>
#include <QUndoStack>
#include <QWheelEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
class WheelProbeItem : public QGraphicsRectItem
{
public:
    using QGraphicsRectItem::QGraphicsRectItem;

    bool receivedWheel() const { return m_receivedWheel; }

protected:
    void wheelEvent(QGraphicsSceneWheelEvent *event) override
    {
        m_receivedWheel = true;
        QGraphicsRectItem::wheelEvent(event);
    }

private:
    bool m_receivedWheel{false};
};

bool windowContainsColor(WId window_id, const QRect &area, const QColor &expected, int minimum_pixels = 1)
{
    const QImage image = QGuiApplication::primaryScreen()->grabWindow(window_id).toImage();
    int          matches = 0;
    for (int y = area.top(); y <= area.bottom() && y < image.height(); ++y) {
        for (int x = area.left(); x <= area.right() && x < image.width(); ++x) {
            if (image.pixelColor(x, y) == expected) {
                ++matches;
                if (matches >= minimum_pixels) {
                    return true;
                }
            }
        }
    }
    return false;
}

#ifdef Q_OS_WIN
void nativeClick(const QPoint &global_position)
{
    SetCursorPos(global_position.x(), global_position.y());
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void nativeDrag(const QPoint &global_start, const QPoint &global_end)
{
    SetCursorPos(global_start.x(), global_start.y());
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    for (int step = 1; step <= 8; ++step) {
        const QPoint point = global_start + (global_end - global_start) * step / 8;
        SetCursorPos(point.x(), point.y());
        QTest::qWait(15);
    }
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}
#endif
} // namespace

class CaptureOverlayTest : public QObject
{
    Q_OBJECT

private slots:
    void entersAnnotationWithoutSelectedTool();
    void toolbarUsesArrowCursor();
    void annotationHandlesUseDirectionalCursors();
    void edgeHandlesResizeSingleAxis();
    void itemResizeRequestsRepaintDuringDrag();
    void cursorReflectsAnnotationContext();
    void blocksAnnotationInputOutsideSelection();
    void idleModeMovesSelectionWithoutCreatingAnnotation();
    void editsNewestAnnotationBeforeAnotherOperation();
    void doesNotReselectFrozenAnnotation();
    void rightClickLeavesAnnotationEditingWithoutCancellingCapture();
    void copiesDisplayedRgbColorInSelectionPhase();
    void shiftSwitchesCopiedColorToHex();
    void selectionWheelDoesNotReachScene();
    void wheelUpdatesMosaicBrushWidthInToolbar();
    void startsPointCaptureWithEmptySelection();
    void lastAreaModeRestoresRecordedSelection();
    void switchesFromSelectionToAnnotationAndRendersOutput();
    void drawsRectangleFromMouseInput();
    void previewsAreaEffectsBeforeMouseRelease();
    void drawsAfterMouseSelectionAndToolClick();
    void drawsRectangleFromNativeWindowsInput();
    void createsEveryAnnotationToolFromViewportInput_data();
    void createsEveryAnnotationToolFromViewportInput();
    void repaintsTypedTextInViewport();
    void annotationButtonsUseIconsWithoutText();
};

void CaptureOverlayTest::entersAnnotationWithoutSelectedTool()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 300, 250));
    overlay.enterAnnotating();

    QCOMPARE(overlay.paintScene()->tool(), PaintTool::kNone);
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").isValid()) {
            QVERIFY(!button->isChecked());
        }
    }
    overlay.hide();
}

void CaptureOverlayTest::toolbarUsesArrowCursor()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 400, 320));
    overlay.enterAnnotating();

    auto *paint_bar = overlay.findChild<PaintBar *>();
    QVERIFY(paint_bar);
    QCOMPARE(paint_bar->cursor().shape(), Qt::ArrowCursor);
    for (QWidget *child : paint_bar->findChildren<QWidget *>()) {
        QCOMPARE(child->cursor().shape(), Qt::ArrowCursor);
    }
    overlay.hide();
}

void CaptureOverlayTest::annotationHandlesUseDirectionalCursors()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 400, 320));
    overlay.enterAnnotating();
    PaintScene *scene = overlay.paintScene();
    scene->setTool(PaintTool::kRectangle);
    PaintItem *item = scene->createItemAt(QPointF(150, 150));
    scene->updateActiveItem(QPointF(350, 270));
    scene->finishActiveItem();
    QVERIFY(item->isSelected());

    const QRectF bounds = item->mapRectToScene(item->boundingRect());
    const auto move_to = [&overlay](const QPointF &position) {
        const QPoint point = position.toPoint();
        QMouseEvent event(QEvent::MouseMove,
                          QPointF(point),
                          QPointF(overlay.viewport()->mapToGlobal(point)),
                          Qt::NoButton,
                          Qt::NoButton,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(overlay.viewport(), &event);
    };

    move_to(bounds.topLeft());
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::SizeFDiagCursor);
    move_to(bounds.topRight());
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::SizeBDiagCursor);
    move_to(bounds.center() + QPointF(bounds.width() / 2.0, 0));
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::SizeHorCursor);
    move_to(bounds.center() + QPointF(0, -bounds.height() / 2.0));
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::SizeVerCursor);
    move_to(QPointF(bounds.left() + bounds.width() / 4.0, bounds.top()));
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::SizeAllCursor);
    overlay.hide();
}

void CaptureOverlayTest::edgeHandlesResizeSingleAxis()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 400, 320));
    overlay.enterAnnotating();
    PaintScene *scene = overlay.paintScene();
    scene->setTool(PaintTool::kRectangle);
    PaintItem *item = scene->createItemAt(QPointF(150, 150));
    scene->updateActiveItem(QPointF(350, 270));
    scene->finishActiveItem();
    QVERIFY(item->isSelected());

    const QRectF original = item->mapRectToScene(item->boundingRect());
    const QPoint right_handle(qRound(original.right()), qRound(original.center().y()));
    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, right_handle);
    QMouseEvent move_event(QEvent::MouseMove,
                           QPointF(right_handle + QPoint(60, 0)),
                           QPointF(overlay.viewport()->mapToGlobal(right_handle + QPoint(60, 0))),
                           Qt::NoButton,
                           Qt::LeftButton,
                           Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &move_event);
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, right_handle + QPoint(60, 0));

    const QRectF resized = item->mapRectToScene(item->boundingRect());
    QVERIFY(resized.width() > original.width() + 30);
    QVERIFY(qAbs(resized.height() - original.height()) < 1);
    QVERIFY(qAbs(resized.left() - original.left()) < 1);

    scene->undoStack()->undo();
    const QRectF restored_after_horizontal = item->mapRectToScene(item->boundingRect());
    QVERIFY(qAbs(restored_after_horizontal.width() - original.width()) < 1);
    QVERIFY(qAbs(restored_after_horizontal.height() - original.height()) < 1);

    const QPoint bottom_handle(qRound(restored_after_horizontal.center().x()),
                               qRound(restored_after_horizontal.bottom()));
    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, bottom_handle);
    QMouseEvent vertical_move(QEvent::MouseMove,
                              QPointF(bottom_handle + QPoint(0, 45)),
                              QPointF(overlay.viewport()->mapToGlobal(bottom_handle + QPoint(0, 45))),
                              Qt::NoButton,
                              Qt::LeftButton,
                              Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &vertical_move);
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, bottom_handle + QPoint(0, 45));

    const QRectF resized_vertically = item->mapRectToScene(item->boundingRect());
    QVERIFY(resized_vertically.height() > original.height() + 20);
    QVERIFY(qAbs(resized_vertically.width() - original.width()) < 1);
    QVERIFY(qAbs(resized_vertically.top() - original.top()) < 1);
    overlay.hide();
}

void CaptureOverlayTest::itemResizeRequestsRepaintDuringDrag()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 400, 320));
    overlay.enterAnnotating();
    PaintScene *scene = overlay.paintScene();
    scene->setTool(PaintTool::kRectangle);
    PaintItem *item = scene->createItemAt(QPointF(150, 150));
    scene->updateActiveItem(QPointF(350, 270));
    scene->finishActiveItem();

    const QRectF bounds = item->mapRectToScene(item->boundingRect());
    const QPoint right_handle(qRound(bounds.right()), qRound(bounds.center().y()));
    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, right_handle);

    QSignalSpy repaint_spy(scene, &PaintScene::sigRepaintRequested);
    QMouseEvent move_event(QEvent::MouseMove,
                           QPointF(right_handle + QPoint(40, 0)),
                           QPointF(overlay.viewport()->mapToGlobal(right_handle + QPoint(40, 0))),
                           Qt::NoButton,
                           Qt::LeftButton,
                           Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &move_event);

    QVERIFY(repaint_spy.count() >= 1);
    overlay.hide();
}

void CaptureOverlayTest::cursorReflectsAnnotationContext()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 300, 250));
    overlay.enterAnnotating();

    const auto move_to = [&overlay](const QPoint &position) {
        QMouseEvent event(QEvent::MouseMove,
                          QPointF(position),
                          QPointF(overlay.viewport()->mapToGlobal(position)),
                          Qt::NoButton,
                          Qt::NoButton,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(overlay.viewport(), &event);
    };

    move_to(QPoint(200, 200));
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::SizeAllCursor);
    move_to(QPoint(50, 200));
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::SizeHorCursor);
    move_to(QPoint(200, 50));
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::SizeVerCursor);

    QToolButton *rectangle_button = nullptr;
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").toInt() == static_cast<int>(PaintTool::kRectangle)) {
            rectangle_button = button;
            break;
        }
    }
    QVERIFY(rectangle_button);
    QTest::mouseClick(rectangle_button, Qt::LeftButton);
    move_to(QPoint(200, 200));
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::CrossCursor);
    move_to(QPoint(50, 200));
    QCOMPARE(overlay.viewport()->cursor().shape(), Qt::ForbiddenCursor);
    overlay.hide();
}

void CaptureOverlayTest::blocksAnnotationInputOutsideSelection()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    const QRect selection(100, 100, 300, 250);
    overlay.setSelection(selection);
    overlay.enterAnnotating();

    QToolButton *rectangle_button = nullptr;
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").toInt() == static_cast<int>(PaintTool::kRectangle)) {
            rectangle_button = button;
            break;
        }
    }
    QVERIFY(rectangle_button);
    QTest::mouseClick(rectangle_button, Qt::LeftButton);

    QSignalSpy cancel_spy(&overlay, &CaptureOverlay::sigCancelRequested);
    QTest::mouseClick(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(50, 200));
    QTest::mouseClick(overlay.viewport(), Qt::RightButton, Qt::NoModifier, QPoint(50, 200));

    QCOMPARE(overlay.phase(), CapturePhase::kAnnotating);
    QCOMPARE(overlay.currentSelection(), selection);
    QCOMPARE(overlay.paintScene()->visibleItemCount(), 0);
    QCOMPARE(cancel_spy.count(), 0);
    overlay.hide();
}

void CaptureOverlayTest::idleModeMovesSelectionWithoutCreatingAnnotation()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 300, 250));
    overlay.enterAnnotating();

    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(200, 200));
    QMouseEvent move_event(QEvent::MouseMove,
                           QPointF(220, 230),
                           QPointF(overlay.viewport()->mapToGlobal(QPoint(220, 230))),
                           Qt::NoButton,
                           Qt::LeftButton,
                           Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &move_event);
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(220, 230));

    QCOMPARE(overlay.currentSelection(), QRect(120, 130, 300, 250));
    QCOMPARE(overlay.paintScene()->visibleItemCount(), 0);
    overlay.hide();
}

void CaptureOverlayTest::editsNewestAnnotationBeforeAnotherOperation()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 400, 320));
    overlay.enterAnnotating();

    QToolButton *rectangle_button = overlay.findChild<QToolButton *>(QStringLiteral("btn_ToolRectangle"));
    QVERIFY(rectangle_button);
    QTest::mouseClick(rectangle_button, Qt::LeftButton);

    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(150, 150));
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(300, 260));
    PaintScene *scene = overlay.paintScene();
    QCOMPARE(scene->visibleItemCount(), 1);
    auto *item = dynamic_cast<PaintItem *>(scene->items().constFirst());
    QVERIFY(item);
    QVERIFY(item->isSelected());

    const QRectF original_bounds = item->mapRectToScene(item->boundingRect());
    const QPointF original_center = original_bounds.center();
    const QPoint move_start(qRound(original_bounds.left() + original_bounds.width() / 4.0),
                            qRound(original_bounds.top()));
    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, move_start);
    QMouseEvent move_event(QEvent::MouseMove,
                           QPointF(move_start + QPoint(40, 30)),
                           QPointF(overlay.viewport()->mapToGlobal(move_start + QPoint(40, 30))),
                           Qt::NoButton,
                           Qt::LeftButton,
                           Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &move_event);
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, move_start + QPoint(40, 30));
    QVERIFY(item->mapRectToScene(item->boundingRect()).center() != original_center);

    const QRectF before_resize = item->mapRectToScene(item->boundingRect());
    const QPoint handle = before_resize.bottomRight().toPoint();
    const qreal original_width = before_resize.width();
    const qreal original_height = before_resize.height();
    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, handle);
    QMouseEvent resize_event(QEvent::MouseMove,
                             QPointF(handle + QPoint(60, 40)),
                             QPointF(overlay.viewport()->mapToGlobal(handle + QPoint(60, 40))),
                             Qt::NoButton,
                             Qt::LeftButton,
                             Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &resize_event);
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, handle + QPoint(60, 40));
    const QRectF after_resize = item->mapRectToScene(item->boundingRect());
    QVERIFY(after_resize.width() > original_width);
    QVERIFY(after_resize.height() > original_height);

    QTest::mouseClick(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(130, 130));
    QCOMPARE(scene->visibleItemCount(), 1);
    QCOMPARE(scene->selectedItems().size(), 0);
    overlay.hide();
}

void CaptureOverlayTest::doesNotReselectFrozenAnnotation()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 400, 320));
    overlay.enterAnnotating();

    QToolButton *rectangle_button = overlay.findChild<QToolButton *>(QStringLiteral("btn_ToolRectangle"));
    QVERIFY(rectangle_button);
    QTest::mouseClick(rectangle_button, Qt::LeftButton);

    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(150, 150));
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(300, 260));
    PaintScene *scene = overlay.paintScene();
    QCOMPARE(scene->visibleItemCount(), 1);

    QTest::mouseClick(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(430, 390));
    QCOMPARE(scene->selectedItems().size(), 0);
    QTest::mouseClick(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(200, 180));
    QCOMPARE(scene->selectedItems().size(), 0);
    overlay.hide();
}

void CaptureOverlayTest::rightClickLeavesAnnotationEditingWithoutCancellingCapture()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 400, 320));
    overlay.enterAnnotating();

    QToolButton *rectangle_button = overlay.findChild<QToolButton *>(QStringLiteral("btn_ToolRectangle"));
    QVERIFY(rectangle_button);
    QTest::mouseClick(rectangle_button, Qt::LeftButton);

    PaintScene *scene = overlay.paintScene();
    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(150, 150));
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(300, 260));
    QCOMPARE(scene->selectedItems().size(), 1);

    QSignalSpy cancel_spy(&overlay, &CaptureOverlay::sigCancelRequested);
    QTest::mouseClick(overlay.viewport(), Qt::RightButton, Qt::NoModifier, QPoint(430, 390));

    QCOMPARE(scene->selectedItems().size(), 0);
    QCOMPARE(cancel_spy.count(), 0);
    QCOMPARE(overlay.phase(), CapturePhase::kAnnotating);
    overlay.hide();
}

void CaptureOverlayTest::copiesDisplayedRgbColorInSelectionPhase()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(12, 34, 56));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.show();
    overlay.activateWindow();
    overlay.setFocus();
    const QPointF     pointer_position(150, 150);
    QMouseEvent pointer_move(QEvent::MouseMove,
                             pointer_position,
                             overlay.viewport()->mapToGlobal(pointer_position.toPoint()),
                             Qt::NoButton,
                             Qt::NoButton,
                             Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &pointer_move);
    QGuiApplication::clipboard()->clear();

    QTest::keyClick(overlay.viewport(), Qt::Key_C);

    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("12, 34, 56"));
    overlay.hide();
}

void CaptureOverlayTest::shiftSwitchesCopiedColorToHex()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(12, 34, 56));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.show();
    overlay.activateWindow();
    overlay.setFocus();
    const QPointF pointer_position(150, 150);
    QMouseEvent pointer_move(QEvent::MouseMove,
                             pointer_position,
                             overlay.viewport()->mapToGlobal(pointer_position.toPoint()),
                             Qt::NoButton,
                             Qt::NoButton,
                             Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &pointer_move);
    QGuiApplication::clipboard()->clear();

    QTest::keyClick(overlay.viewport(), Qt::Key_Shift);
    QTest::keyClick(overlay.viewport(), Qt::Key_C);

    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("#0C2238"));

    QTest::keyClick(overlay.viewport(), Qt::Key_Shift);
    QTest::keyClick(overlay.viewport(), Qt::Key_C);
    QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("12, 34, 56"));
    overlay.hide();
}

void CaptureOverlayTest::selectionWheelDoesNotReachScene()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    auto *probe = new WheelProbeItem(QRectF(0, 0, 200, 200));
    probe->setPos(100, 100);
    overlay.paintScene()->addItem(probe);

    const QPointF local_position(150, 150);
    QWheelEvent wheel_event(local_position,
                            overlay.viewport()->mapToGlobal(local_position.toPoint()),
                            QPoint(),
                            QPoint(0, 120),
                            Qt::NoButton,
                            Qt::NoModifier,
                            Qt::NoScrollPhase,
                            false);
    QCoreApplication::sendEvent(overlay.viewport(), &wheel_event);

    QVERIFY(!probe->receivedWheel());
    QVERIFY(wheel_event.isAccepted());
    overlay.hide();
}

void CaptureOverlayTest::wheelUpdatesMosaicBrushWidthInToolbar()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 500, 400));
    overlay.enterAnnotating();

    QToolButton *mosaic_button = nullptr;
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").toInt() == static_cast<int>(PaintTool::kMosaic)) {
            mosaic_button = button;
            break;
        }
    }
    auto *level = overlay.findChild<QSlider *>(QStringLiteral("slider_EffectLevel"));
    QVERIFY(mosaic_button);
    QVERIFY(level);
    QTest::mouseClick(mosaic_button, Qt::LeftButton);
    QCOMPARE(overlay.paintScene()->penWidth(), 14.0);
    QCOMPARE(level->value(), 14);

    level->setValue(64);
    QCOMPARE(overlay.paintScene()->penWidth(), 64.0);

    const QPointF local_position(400, 300);
    QWheelEvent wheel_event(local_position,
                            overlay.viewport()->mapToGlobal(local_position.toPoint()),
                            QPoint(),
                            QPoint(0, -120),
                            Qt::NoButton,
                            Qt::NoModifier,
                            Qt::NoScrollPhase,
                            false);
    QCoreApplication::sendEvent(overlay.viewport(), &wheel_event);

    QCOMPARE(overlay.paintScene()->penWidth(), 48.0);
    QCOMPARE(level->value(), 48);
    overlay.hide();
}

void CaptureOverlayTest::startsPointCaptureWithEmptySelection()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 120, 320, 240));
    overlay.hide();

    CaptureRequest request;
    request.startMode = CaptureStartMode::kPoint;
    overlay.begin(frame, options, request);

    QVERIFY(!overlay.currentSelection().isValid());
    overlay.hide();
}

void CaptureOverlayTest::lastAreaModeRestoresRecordedSelection()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    const QRect recorded(80, 90, 360, 220);
    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(recorded);
    overlay.recordSelectionForHistory();
    overlay.hide();

    CaptureRequest request;
    request.startMode = CaptureStartMode::kLastArea;
    overlay.begin(frame, options, request);

    QCOMPARE(overlay.currentSelection(), recorded);
    overlay.hide();
}

void CaptureOverlayTest::switchesFromSelectionToAnnotationAndRendersOutput()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    auto *paint_bar = overlay.findChild<PaintBar *>();
    QVERIFY(paint_bar);
    QVERIFY(!paint_bar->isVisible());

    const QRect selection(100, 100, 200, 150);
    overlay.setSelection(selection);
    overlay.enterAnnotating();
    QCOMPARE(overlay.phase(), CapturePhase::kAnnotating);
    QVERIFY(paint_bar->isVisible());

    PaintScene *scene = overlay.paintScene();
    scene->setColor(QColor(255, 82, 82));
    scene->setFilled(true);
    scene->setTool(PaintTool::kRectangle);
    scene->createItemAt(QPointF(110, 110));
    scene->updateActiveItem(QPointF(290, 240));
    scene->finishActiveItem();

    const QImage output = overlay.renderSelection();
    QCOMPARE(output.size(), selection.size());
    QCOMPARE(output.pixelColor(5, 5), frame.logicalComposite.pixelColor(105, 105));
    QCOMPARE(output.pixelColor(100, 75), QColor(255, 82, 82));

    QString error;
    QVERIFY2(OutputService::copyImage(output, &error), qPrintable(error));
    QCOMPARE(QGuiApplication::clipboard()->image().size(), output.size());

    overlay.paintScene()->clearAll();
    overlay.hide();
}

void CaptureOverlayTest::drawsRectangleFromMouseInput()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 300, 250));
    overlay.enterAnnotating();

    QToolButton *rectangle_button = nullptr;
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").toInt() == static_cast<int>(PaintTool::kRectangle)) {
            rectangle_button = button;
            break;
        }
    }
    QVERIFY(rectangle_button);
    QTest::mouseClick(rectangle_button, Qt::LeftButton);

    PaintScene *scene = overlay.paintScene();
    scene->setColor(QColor(255, 82, 82));
    scene->setFilled(true);
    QCOMPARE(scene->tool(), PaintTool::kRectangle);

    const QPoint start(150, 150);
    const QPoint end(300, 260);
    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, start);
    QMouseEvent move_event(QEvent::MouseMove,
                           QPointF(end),
                           QPointF(overlay.viewport()->mapToGlobal(end)),
                           Qt::NoButton,
                           Qt::LeftButton,
                           Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &move_event);
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, end);

    QCOMPARE(scene->visibleItemCount(), 1);
    const QImage viewport_image = overlay.viewport()->grab().toImage();
    QCOMPARE(viewport_image.pixelColor(start + QPoint(20, 20)), QColor(255, 82, 82));
    const QImage output = overlay.renderSelection();
    QCOMPARE(output.pixelColor(100, 100), QColor(255, 82, 82));

    scene->clearAll();
    overlay.hide();
}

void CaptureOverlayTest::previewsAreaEffectsBeforeMouseRelease()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32);
    for (int y = 0; y < frame.logicalComposite.height(); ++y) {
        for (int x = 0; x < frame.logicalComposite.width(); ++x) {
            frame.logicalComposite.setPixelColor(x, y, (x + y) % 2 == 0 ? Qt::white : Qt::black);
        }
    }
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 500, 400));
    overlay.enterAnnotating();

    QToolButton *mosaic_button = nullptr;
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").toInt() == static_cast<int>(PaintTool::kMosaic)) {
            mosaic_button = button;
            break;
        }
    }
    auto *area_button = overlay.findChild<QToolButton *>(QStringLiteral("btn_MosaicArea"));
    QVERIFY(mosaic_button);
    QVERIFY(area_button);
    QTest::mouseClick(mosaic_button, Qt::LeftButton);
    QTest::mouseClick(area_button, Qt::LeftButton);

    PaintScene *scene = overlay.paintScene();
    QCOMPARE(scene->tool(), PaintTool::kMosaic);
    QVERIFY(scene->isFilled());
    scene->setEffectLevel(12);

    for (PaintEffectMode effect_mode : {PaintEffectMode::kMosaic, PaintEffectMode::kBlur}) {
        scene->setEffectMode(effect_mode);
        scene->setEffectLevel(12);
        const QPoint start(200, 200);
        const QPoint end(360, 320);
        QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, start);
        QMouseEvent move_event(QEvent::MouseMove,
                               QPointF(end),
                               QPointF(overlay.viewport()->mapToGlobal(end)),
                               Qt::NoButton,
                               Qt::LeftButton,
                               Qt::NoModifier);
        QCoreApplication::sendEvent(overlay.viewport(), &move_event);
        QCoreApplication::processEvents();

        QCOMPARE(scene->visibleItemCount(), 1);
        auto *active_item = dynamic_cast<PaintItem *>(scene->items().constFirst());
        QVERIFY(active_item);
        QCOMPARE(active_item->points().size(), 2);
        QVERIFY(active_item->isFilled());
        QCOMPARE(active_item->effectMode(), effect_mode);
        QCOMPARE(active_item->effectLevel(), 12);
        const QImage preview = overlay.viewport()->grab().toImage();
        const QImage rendered = overlay.renderSelection();
        int          changed_pixels = 0;
        int          rendered_changed_pixels = 0;
        for (int y = start.y() + 20; y < end.y() - 10; y += 4) {
            for (int x = start.x() + 20; x < end.x() - 10; x += 4) {
                if (preview.pixelColor(x, y) != frame.logicalComposite.pixelColor(x, y)) {
                    ++changed_pixels;
                }
                if (rendered.pixelColor(x - 100, y - 100)
                    != frame.logicalComposite.pixelColor(x, y)) {
                    ++rendered_changed_pixels;
                }
            }
        }
        QVERIFY2(changed_pixels > 100,
                 qPrintable(QStringLiteral("changed_pixels=%1 rendered_changed_pixels=%2")
                                .arg(changed_pixels)
                                .arg(rendered_changed_pixels)));

        QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, end);
        scene->clearAll();
    }

    overlay.hide();
}

void CaptureOverlayTest::drawsAfterMouseSelectionAndToolClick()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    connect(&overlay, &CaptureOverlay::sigSelectionConfirmed, &overlay, &CaptureOverlay::enterAnnotating);
    overlay.begin(frame, options, CaptureRequest{});

    const QPoint selection_start(100, 100);
    const QPoint selection_end(400, 350);
    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, selection_start);
    QMouseEvent selection_move(QEvent::MouseMove,
                               QPointF(selection_end),
                               QPointF(overlay.viewport()->mapToGlobal(selection_end)),
                               Qt::NoButton,
                               Qt::LeftButton,
                               Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &selection_move);
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, selection_end);
    QCOMPARE(overlay.phase(), CapturePhase::kAnnotating);
    QCOMPARE(overlay.currentSelection(), QRect(100, 100, 301, 251));

    QToolButton *rectangle_button = nullptr;
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").toInt() == static_cast<int>(PaintTool::kRectangle)) {
            rectangle_button = button;
            break;
        }
    }
    QVERIFY(rectangle_button);
    QTest::mouseClick(rectangle_button, Qt::LeftButton);

    PaintScene *scene = overlay.paintScene();
    scene->setColor(QColor(255, 82, 82));
    scene->setFilled(true);
    const QPoint draw_start(150, 150);
    const QPoint draw_end(300, 260);
    QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, draw_start);
    QMouseEvent draw_move(QEvent::MouseMove,
                          QPointF(draw_end),
                          QPointF(overlay.viewport()->mapToGlobal(draw_end)),
                          Qt::NoButton,
                          Qt::LeftButton,
                          Qt::NoModifier);
    QCoreApplication::sendEvent(overlay.viewport(), &draw_move);
    QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, draw_end);

    QCOMPARE(scene->visibleItemCount(), 1);
    QCOMPARE(overlay.renderSelection().pixelColor(100, 100), QColor(255, 82, 82));

    scene->clearAll();
    overlay.hide();
}

void CaptureOverlayTest::drawsRectangleFromNativeWindowsInput()
{
#ifndef Q_OS_WIN
    QSKIP("Windows native input test");
#else
    if (QGuiApplication::platformName() != QStringLiteral("windows")) {
        QSKIP("Requires the Windows platform plugin");
    }

    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 400, 400));
    overlay.enterAnnotating();
    overlay.raise();
    overlay.activateWindow();
    QTest::qWait(250);

    QToolButton *rectangle_button = nullptr;
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").toInt() == static_cast<int>(PaintTool::kRectangle)) {
            rectangle_button = button;
            break;
        }
    }
    QVERIFY(rectangle_button);
    nativeClick(rectangle_button->mapToGlobal(rectangle_button->rect().center()));
    QTest::qWait(100);

    PaintScene *scene = overlay.paintScene();
    scene->setColor(QColor(255, 82, 82));
    scene->setFilled(true);
    QCOMPARE(scene->tool(), PaintTool::kRectangle);

    nativeDrag(overlay.viewport()->mapToGlobal(QPoint(200, 200)), overlay.viewport()->mapToGlobal(QPoint(400, 300)));
    QTest::qWait(300);

    QCOMPARE(scene->visibleItemCount(), 1);
    QTRY_VERIFY_WITH_TIMEOUT(windowContainsColor(overlay.winId(), QRect(220, 220, 1, 1), QColor(255, 82, 82)), 2000);
    scene->clearAll();
    overlay.hide();
#endif
}

void CaptureOverlayTest::createsEveryAnnotationToolFromViewportInput_data()
{
    QTest::addColumn<int>("tool");
    QTest::newRow("rectangle") << static_cast<int>(PaintTool::kRectangle);
    QTest::newRow("ellipse") << static_cast<int>(PaintTool::kEllipse);
    QTest::newRow("line") << static_cast<int>(PaintTool::kLine);
    QTest::newRow("arrow") << static_cast<int>(PaintTool::kArrow);
    QTest::newRow("pencil") << static_cast<int>(PaintTool::kPencil);
    QTest::newRow("marker") << static_cast<int>(PaintTool::kMarker);
    QTest::newRow("mosaic") << static_cast<int>(PaintTool::kMosaic);
    QTest::newRow("text") << static_cast<int>(PaintTool::kText);
    QTest::newRow("number") << static_cast<int>(PaintTool::kNumber);
    QTest::newRow("eraser") << static_cast<int>(PaintTool::kEraser);
}

void CaptureOverlayTest::createsEveryAnnotationToolFromViewportInput()
{
    QFETCH(int, tool);
    const auto paint_tool = static_cast<PaintTool>(tool);

    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 500, 400));
    overlay.enterAnnotating();

    QToolButton *tool_button = nullptr;
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").toInt() == tool) {
            tool_button = button;
            break;
        }
    }
    QVERIFY(tool_button);
    QTest::mouseClick(tool_button, Qt::LeftButton);

    PaintScene *scene = overlay.paintScene();
    QCOMPARE(scene->tool(), paint_tool);
    scene->setFilled(true);

    const QPoint start(200, 200);
    if (paint_tool == PaintTool::kText || paint_tool == PaintTool::kNumber) {
        QTest::mouseClick(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, start);
    } else {
        const QPoint end(400, 320);
        QTest::mousePress(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, start);
        QMouseEvent move_event(QEvent::MouseMove,
                               QPointF(end),
                               QPointF(overlay.viewport()->mapToGlobal(end)),
                               Qt::NoButton,
                               Qt::LeftButton,
                               Qt::NoModifier);
        QCoreApplication::sendEvent(overlay.viewport(), &move_event);
        QTest::mouseRelease(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, end);
    }
    QTest::qWait(20);

    QCOMPARE(scene->visibleItemCount(), 1);
    QVERIFY(!overlay.renderSelection().isNull());
    scene->clearAll();
    overlay.hide();
}

void CaptureOverlayTest::repaintsTypedTextInViewport()
{
    CaptureFrame frame;
    frame.logicalBounds = QRect(0, 0, 800, 600);
    frame.logicalComposite = QImage(800, 600, QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(QColor(30, 60, 90));
    frame.displays = {
        DisplaySurface{QStringLiteral("test"), frame.logicalBounds, frame.logicalComposite, 1.0},
    };

    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;

    CaptureOverlay overlay;
    overlay.begin(frame, options, CaptureRequest{});
    overlay.setSelection(QRect(100, 100, 500, 400));
    overlay.enterAnnotating();

    QToolButton *text_button = nullptr;
    for (QToolButton *button : overlay.findChildren<QToolButton *>()) {
        if (button->property("paintTool").toInt() == static_cast<int>(PaintTool::kText)) {
            text_button = button;
            break;
        }
    }
    QVERIFY(text_button);
    QTest::mouseClick(text_button, Qt::LeftButton);
    QTest::mouseClick(overlay.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(200, 200));

    auto *text_item = dynamic_cast<QGraphicsTextItem *>(overlay.paintScene()->focusItem());
    QVERIFY(text_item);
    QTest::keyClicks(overlay.viewport(), QStringLiteral("QSnap"));
    QCOMPARE(text_item->toPlainText(), QStringLiteral("QSnap"));
    QTest::qWait(100);

    QTRY_VERIFY_WITH_TIMEOUT(windowContainsColor(overlay.winId(), QRect(190, 190, 210, 60), QColor(255, 82, 82), 6),
                             2000);

    overlay.paintScene()->clearAll();
    overlay.hide();
}

void CaptureOverlayTest::annotationButtonsUseIconsWithoutText()
{
    CaptureOverlay             overlay;
    const QList<QToolButton *> buttons = overlay.findChildren<QToolButton *>();
    QVERIFY(!buttons.isEmpty());

    for (QToolButton *button : buttons) {
        if (button->objectName().isEmpty()) {
            continue;
        }
        if (button->property("paletteColor").isValid()) {
            QVERIFY(button->property("paletteColor").value<QColor>().isValid());
            QVERIFY(!button->toolTip().isEmpty());
            continue;
        }
        QVERIFY2(!button->icon().isNull(), qPrintable(button->objectName()));
        QVERIFY2(button->text().isEmpty(), qPrintable(button->objectName()));
        QVERIFY2(!button->toolTip().isEmpty(), qPrintable(button->objectName()));
    }

    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (!render_directory.isEmpty()) {
        auto *paint_bar = overlay.findChild<PaintBar *>();
        QVERIFY(paint_bar);
        const QPixmap image = paint_bar->grab();
        QVERIFY(!image.isNull());
        QVERIFY(image.save(render_directory + QStringLiteral("/paint-bar-icons.png")));
    }
}

QTEST_MAIN(CaptureOverlayTest)

#include "captureoverlaytest.moc"
