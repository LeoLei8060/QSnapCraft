#include "captureoverlay.h"

#include "paint/paintscene.h"
#include "ui/paintbar.h"

#include <QClipboard>
#include <QCursor>
#include <QGraphicsTextItem>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QUndoStack>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace {
constexpr int HOVER_THROTTLE_MS = 16;
constexpr int DRAG_THRESHOLD_PX = 3;
constexpr int ANCHOR_SIZE_PX = 6;
constexpr int ANCHOR_HIT_PX = 6;
const QColor  MASK_COLOR(0, 0, 0, 110);
const QColor  BORDER_COLOR(255, 255, 255);
const QColor  HOVER_COLOR(0, 174, 255);
} // namespace

CaptureOverlay::CaptureOverlay(QWidget *parent)
    : PaintView(parent)
    , m_selectionState(QRect())
    , m_history(30, true)
    , m_detector(nullptr)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::CrossCursor);

    m_hoverTimer.setInterval(HOVER_THROTTLE_MS);
    connect(&m_hoverTimer, &QTimer::timeout, this, &CaptureOverlay::updateHover);

    m_paintBar = new PaintBar(this);
    m_paintBar->hide();
    connect(m_paintBar, &PaintBar::sigToolChanged, this, [this](PaintTool tool) {
        paintScene()->setTool(tool);
        m_paintBar->setCurrentColor(paintScene()->color());
        updatePaintBarPosition();
        updateInteractionCursor();
    });
    connect(m_paintBar, &PaintBar::sigColorChanged, paintScene(), &PaintScene::setColor);
    connect(m_paintBar, &PaintBar::sigFilledChanged, paintScene(), &PaintScene::setFilled);
    connect(m_paintBar, &PaintBar::sigPenWidthChanged, paintScene(), &PaintScene::setPenWidth);
    connect(paintScene(), &PaintScene::sigPenWidthChanged, m_paintBar, &PaintBar::setCurrentPenWidth);
    connect(m_paintBar, &PaintBar::sigArrowStyleChanged, paintScene(), &PaintScene::setArrowStyle);
    connect(m_paintBar, &PaintBar::sigMarkerOpacityChanged, paintScene(), &PaintScene::setMarkerOpacity);
    connect(m_paintBar, &PaintBar::sigEffectModeChanged, paintScene(), &PaintScene::setEffectMode);
    connect(m_paintBar, &PaintBar::sigEffectLevelChanged, paintScene(), &PaintScene::setEffectLevel);
    connect(m_paintBar, &PaintBar::sigFontFamilyChanged, paintScene(), &PaintScene::setFontFamily);
    connect(m_paintBar, &PaintBar::sigFontSizeChanged, paintScene(), &PaintScene::setFontSize);
    connect(m_paintBar, &PaintBar::sigFontBoldChanged, paintScene(), &PaintScene::setFontBold);
    connect(m_paintBar, &PaintBar::sigFontItalicChanged, paintScene(), &PaintScene::setFontItalic);
    connect(m_paintBar, &PaintBar::sigTextOutlinedChanged, paintScene(), &PaintScene::setTextOutlined);
    connect(m_paintBar, &PaintBar::sigTextBackgroundChanged, paintScene(), &PaintScene::setTextBackgroundEnabled);
    connect(m_paintBar, &PaintBar::sigCommandRequested, this, [this](CaptureCommand command) {
        if (command == CaptureCommand::kCancel) {
            emit sigCancelRequested();
        } else if (command == CaptureCommand::kUndo) {
            paintScene()->stopItemEditing();
            paintScene()->undoStack()->undo();
        } else if (command == CaptureCommand::kRedo) {
            paintScene()->stopItemEditing();
            paintScene()->undoStack()->redo();
        } else if (command == CaptureCommand::kClearAll) {
            paintScene()->clearAll();
        } else {
            emit sigCommandRequested(command, true);
        }
    });
    connect(paintScene()->undoStack(), &QUndoStack::canUndoChanged, m_paintBar, &PaintBar::setUndoEnabled);
    connect(paintScene()->undoStack(), &QUndoStack::canRedoChanged, m_paintBar, &PaintBar::setRedoEnabled);
}

CaptureOverlay::~CaptureOverlay() = default;

void CaptureOverlay::begin(const CaptureFrame &frame, const CaptureOptions &options, CaptureRequest request)
{
    m_frame = frame;
    m_options = options;
    m_request = std::move(request);
    m_backgroundPixmap = QPixmap::fromImage(frame.logicalComposite);
    m_magnifierVisible = options.showMagnifier;
    m_maskVisible = true;
    m_magnifier.setDecorations(options.showMaskInMagnifier,
                               options.showBorderInMagnifier,
                               options.showAnchorsInMagnifier);
    m_history.setLimit(qMax(1, options.maxHistoryAreas));
    m_history.setLoop(options.loopHistoryAreas);

    setGeometry(frame.logicalBounds);
    setBackgroundImage(frame.logicalComposite);
    m_pointerPosition = QCursor::pos() - frame.logicalBounds.topLeft();
    m_pendingHoverPosition = m_pointerPosition;
    paintScene()->clearAll();
    m_selectionState.setBounds(QRect(QPoint(0, 0), frame.logicalComposite.size()));
    m_selectionState.clear();
    m_hoverHierarchy.clear();
    m_hoverRect = {};
    m_activeEdges = SelectionEdge::kNone;
    m_isDragging = false;
    m_adopting = false;
    m_dragged = false;
    m_isResizing = false;
    m_isMoving = false;
    m_detectionEnabled = true;
    m_detector.refresh();

    setPhase(CapturePhase::kSelecting);
    show();
    raise();
    activateWindow();
    setFocus();
    m_hoverTimer.start();
    applyStartMode();
}

void CaptureOverlay::setPhase(CapturePhase phase)
{
    m_phase = phase;
    const bool selecting = phase == CapturePhase::kSelecting;
    if (selecting) {
        m_hoverTimer.start();
    } else {
        m_hoverTimer.stop();
    }
    if (m_paintBar != nullptr) {
        m_paintBar->setVisible(phase == CapturePhase::kAnnotating);
        if (phase == CapturePhase::kAnnotating) {
            updatePaintBarPosition();
            m_paintBar->raise();
        }
    }
    updateInteractionCursor();
    viewport()->update();
}

CapturePhase CaptureOverlay::phase() const
{
    return m_phase;
}

void CaptureOverlay::setSelection(const QRect &selection)
{
    m_selectionState.setSelection(selection);
    viewport()->update();
}

QRect CaptureOverlay::currentSelection() const
{
    return m_selectionState.current();
}

void CaptureOverlay::enterAnnotating()
{
    if (!m_selectionState.isValid()) {
        return;
    }
    m_hoverRect = {};
    m_hoverHierarchy.clear();
    setPhase(CapturePhase::kAnnotating);
    paintScene()->setTool(PaintTool::kNone);
    m_paintBar->setCurrentTool(PaintTool::kNone);
    m_paintBar->setCurrentColor(paintScene()->color());
    updateInteractionCursor();
}

QImage CaptureOverlay::renderSelection() const
{
    const QRect selection = m_selectionState.current();
    QImage      output;
    QString     error;
    if (!renderCaptureSelection(m_frame, selection, &output, &error)) {
        return {};
    }

    if (!selection.isEmpty()) {
        const QList<QGraphicsItem *> selectedItems = paintScene()->selectedItems();
        paintScene()->clearSelection();
        paintScene()->clearFocus();
        QPainter painter(&output);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        const qreal scaleX = qreal(output.width()) / selection.width();
        const qreal scaleY = qreal(output.height()) / selection.height();
        painter.scale(scaleX, scaleY);
        painter.translate(-selection.left(), -selection.top());
        paintScene()->render(&painter, QRectF(selection), QRectF(selection), Qt::IgnoreAspectRatio);
        for (QGraphicsItem *item : selectedItems) {
            item->setSelected(true);
        }
    }
    return output;
}

void CaptureOverlay::recordSelectionForHistory()
{
    if (m_selectionState.isValid()) {
        m_history.push(m_selectionState.current());
    }
}

void CaptureOverlay::drawOverlay(QPainter &painter)
{
    const QRect selection = m_selectionState.current();
    if (m_phase == CapturePhase::kSelecting) {
        if (m_hoverRect.isValid() && !m_isDragging) {
            drawSelection(painter, m_hoverRect, true);
        }
        if (selection.isValid()) {
            drawSelection(painter, selection, false);
        }
        drawMagnifier(painter);
    } else if (m_phase == CapturePhase::kAnnotating) {
        if (m_maskVisible && m_options.drawOutsideArea && selection.isValid()) {
            QPainterPath outside;
            outside.addRect(viewport()->rect());
            QPainterPath keep;
            keep.addRect(selection);
            painter.fillPath(outside.subtracted(keep), MASK_COLOR);
        }
        drawSelection(painter, selection, false);
        drawAnchors(painter);
        drawSizeLabel(painter);
    }

    if (m_options.autoCrossLines && m_phase == CapturePhase::kSelecting) {
        painter.setPen(QPen(QColor(255, 255, 255, 150), 1, Qt::DashLine));
        painter.drawLine(0, m_pointerPosition.y(), width(), m_pointerPosition.y());
        painter.drawLine(m_pointerPosition.x(), 0, m_pointerPosition.x(), height());
    }
    if (m_options.showTips) {
        drawTips(painter);
    }
}

void CaptureOverlay::mousePressEvent(QMouseEvent *event)
{
    m_pointerPosition = event->position().toPoint();

    if (m_phase == CapturePhase::kAnnotating) {
        const QRect selection = m_selectionState.current();
        const bool  inside_selection = selection.contains(m_pointerPosition);
        const bool  drawing_tool = paintScene()->tool() != PaintTool::kNone;
        if (drawing_tool && !inside_selection
            && (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)) {
            event->accept();
            return;
        }
        if (event->button() == Qt::RightButton) {
            if (paintScene()->hasEditableItem()) {
                paintScene()->stopItemEditing();
                updateInteractionCursor();
                event->accept();
                return;
            }
            emit sigCancelRequested();
            return;
        }
        if (event->button() == Qt::LeftButton && !drawing_tool) {
            if (paintScene()->itemHandleAt(m_pointerPosition) != PaintItemHandle::kNone) {
                PaintView::mousePressEvent(event);
                return;
            }
            SelectionEdges edges = selectionEdgesAt(m_pointerPosition);
            if (edges == SelectionEdge::kNone) {
                edges = selectionEdgesOutsideAt(m_pointerPosition);
            }
            if (edges != SelectionEdge::kNone) {
                paintScene()->stopItemEditing();
                m_activeEdges = edges;
                m_isResizing = true;
                m_selectionState.beginResize(edges, event->modifiers().testFlag(Qt::AltModifier));
                return;
            }
            if (inside_selection) {
                QGraphicsItem *hit_item = paintScene()->itemAt(m_pointerPosition, QTransform());
                if (hit_item != nullptr && hit_item == paintScene()->editableItem()) {
                    PaintView::mousePressEvent(event);
                    return;
                }
                paintScene()->stopItemEditing();
                m_isMoving = true;
                m_selectionState.beginMove(m_pointerPosition);
                updatePaintBarPosition();
                viewport()->update();
                return;
            }
        }
        PaintView::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::RightButton) {
        emit sigCancelRequested();
        return;
    }
    if (event->button() == Qt::MiddleButton) {
        if (m_options.middleClickOutput && m_selectionState.isValid()) {
            emitOutputAction(m_options.middleClickAction, m_options.middleClickExit);
        }
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }

    m_anchor = m_pointerPosition;
    m_dragged = false;
    if (!m_hoverRect.isValid()) {
        m_pendingHoverPosition = m_pointerPosition;
        updateHover();
    }

    m_adopting = m_hoverRect.isValid();
    if (m_adopting) {
        m_selectionState.setSelection(m_hoverRect);
    } else {
        m_selectionState.beginDrag(m_pointerPosition);
    }
    m_isDragging = true;
    viewport()->update();
}

void CaptureOverlay::mouseMoveEvent(QMouseEvent *event)
{
    m_pointerPosition = event->position().toPoint();

    if (m_phase == CapturePhase::kAnnotating) {
        updateInteractionCursor();
        if (m_isResizing && event->buttons().testFlag(Qt::LeftButton)) {
            m_selectionState.resizeTo(m_pointerPosition);
            updatePaintBarPosition();
            viewport()->update();
            return;
        }
        if (m_isMoving && event->buttons().testFlag(Qt::LeftButton)) {
            m_selectionState.moveTo(m_pointerPosition);
            updatePaintBarPosition();
            viewport()->update();
            return;
        }
        PaintView::mouseMoveEvent(event);
        return;
    }

    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_pendingHoverPosition = m_pointerPosition;
        viewport()->update();
        return;
    }

    if (m_adopting && (m_pointerPosition - m_anchor).manhattanLength() > DRAG_THRESHOLD_PX) {
        m_adopting = false;
        m_dragged = true;
        m_selectionState.beginDrag(m_anchor);
    }
    if (!m_adopting) {
        m_selectionState.dragTo(m_pointerPosition);
    }
    viewport()->update();
}

void CaptureOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_phase == CapturePhase::kAnnotating) {
        if (m_isResizing && event->button() == Qt::LeftButton) {
            m_selectionState.endResize();
            m_activeEdges = SelectionEdge::kNone;
            m_isResizing = false;
            updatePaintBarPosition();
            viewport()->update();
            return;
        }
        if (m_isMoving && event->button() == Qt::LeftButton) {
            m_selectionState.moveTo(event->position().toPoint());
            m_selectionState.endMove();
            m_isMoving = false;
            updatePaintBarPosition();
            updateInteractionCursor();
            viewport()->update();
            return;
        }
        PaintView::mouseReleaseEvent(event);
        return;
    }

    if (event->button() != Qt::LeftButton || !m_isDragging) {
        return;
    }

    m_isDragging = false;
    if (!m_adopting) {
        m_selectionState.dragTo(event->position().toPoint());
        m_selectionState.endDrag();
    }
    m_adopting = false;

    if (m_selectionState.isValid()) {
        emit sigSelectionConfirmed(m_selectionState.current());
    }
    viewport()->update();
}

void CaptureOverlay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_phase == CapturePhase::kSelecting && m_options.doubleClickOutput
        && m_selectionState.current().contains(event->position().toPoint())) {
        emitOutputAction(m_options.doubleClickAction, m_options.doubleClickExit);
        return;
    }
    PaintView::mouseDoubleClickEvent(event);
}

void CaptureOverlay::wheelEvent(QWheelEvent *event)
{
    if (m_phase == CapturePhase::kSelecting) {
        event->accept();
        return;
    }
    if (m_phase == CapturePhase::kAnnotating && !m_options.disableMouseScroll && event->angleDelta().y() != 0) {
        const int direction = event->angleDelta().y() > 0 ? 1 : -1;
        paintScene()->adjustPenWidth(direction);
        viewport()->update();
        event->accept();
        return;
    }
    PaintView::wheelEvent(event);
}

void CaptureOverlay::keyPressEvent(QKeyEvent *event)
{
    if (m_phase == CapturePhase::kAnnotating) {
        if (auto *text = dynamic_cast<QGraphicsTextItem *>(paintScene()->focusItem());
            text != nullptr && text->textInteractionFlags().testFlag(Qt::TextEditorInteraction)) {
            if (event->key() == Qt::Key_Escape) {
                text->clearFocus();
                return;
            }
            PaintView::keyPressEvent(event);
            return;
        }
        if (event->key() == Qt::Key_Escape && !paintScene()->selectedItems().isEmpty()) {
            paintScene()->stopItemEditing();
            return;
        }
    }

    if (event->key() == Qt::Key_Escape) {
        if (m_options.askForConfirmOnEscape) {
            const auto answer = QMessageBox::question(this,
                                                      QStringLiteral("QSnapCraft"),
                                                      QStringLiteral("确定要取消本次截图吗？"),
                                                      QMessageBox::Yes | QMessageBox::No,
                                                      QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                return;
            }
        }
        emit sigCancelRequested();
        return;
    }

    if ((m_phase == CapturePhase::kSelecting
         || (m_phase == CapturePhase::kAnnotating && paintScene()->selectedItems().isEmpty()))
        && (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right || event->key() == Qt::Key_Up
            || event->key() == Qt::Key_Down)) {
        m_selectionState.adjustByKeyboard(event->key(),
                                          event->modifiers().testFlag(Qt::ShiftModifier),
                                          event->modifiers().testFlag(Qt::ControlModifier),
                                          event->modifiers().testFlag(Qt::AltModifier));
        viewport()->update();
        return;
    }
    if ((event->key() == Qt::Key_F && event->modifiers() == Qt::NoModifier)
        || (event->key() == Qt::Key_A && event->modifiers().testFlag(Qt::ControlModifier))) {
        m_selectionState.setSelection(QRect(QPoint(0, 0), m_frame.logicalComposite.size()));
        updatePaintBarPosition();
        viewport()->update();
        return;
    }
    if (m_phase == CapturePhase::kSelecting && (event->key() == Qt::Key_Comma || event->key() == Qt::Key_Period)) {
        const bool changed = event->key() == Qt::Key_Period ? m_history.next() : m_history.previous();
        if (changed) {
            m_selectionState.setSelection(m_history.current());
            updatePaintBarPosition();
            viewport()->update();
        }
        return;
    }
    if (event->key() == Qt::Key_Tab) {
        m_detectionEnabled = !m_detectionEnabled;
        updateHover();
        return;
    }
    if (event->key() == Qt::Key_M) {
        m_magnifierVisible = !m_magnifierVisible;
        viewport()->update();
        return;
    }
    if (event->key() == Qt::Key_Shift && m_phase == CapturePhase::kSelecting) {
        m_magnifier.toggleColorFormat();
        viewport()->update();
        return;
    }
    if (event->key() == Qt::Key_S && event->modifiers() == Qt::NoModifier) {
        m_maskVisible = !m_maskVisible;
        viewport()->update();
        return;
    }
    if (event->key() == Qt::Key_C && m_phase == CapturePhase::kSelecting) {
        const QColor color = m_frame.logicalComposite.pixelColor(m_pointerPosition);
        if (color.isValid()) {
            m_magnifier.setCurrentColor(color);
            QString text = m_magnifier.getCurrentColorText();
            if (m_magnifier.isHexFormat() && !m_options.uppercaseHexColor) {
                text = text.toLower();
            }
            QGuiApplication::clipboard()->setText(text);
        }
        return;
    }
    if (m_phase == CapturePhase::kAnnotating) {
        const bool control = event->modifiers().testFlag(Qt::ControlModifier);
        PaintTool  shortcutTool = PaintTool::kSelect;
        bool       hasToolShortcut = true;
        if (control && event->key() == Qt::Key_1)
            shortcutTool = PaintTool::kRectangle;
        else if (control && event->key() == Qt::Key_2)
            shortcutTool = PaintTool::kLine;
        else if (control && event->key() == Qt::Key_3)
            shortcutTool = PaintTool::kArrow;
        else if (control && event->key() == Qt::Key_4)
            shortcutTool = PaintTool::kPencil;
        else if (control && event->key() == Qt::Key_5)
            shortcutTool = PaintTool::kMarker;
        else if (control && event->key() == Qt::Key_6)
            shortcutTool = PaintTool::kMosaic;
        else if (control && event->key() == Qt::Key_7)
            shortcutTool = PaintTool::kText;
        else if (!control && event->key() == Qt::Key_E)
            shortcutTool = PaintTool::kEraser;
        else
            hasToolShortcut = false;
        if (hasToolShortcut) {
            paintScene()->setTool(shortcutTool);
            m_paintBar->setCurrentTool(shortcutTool);
            m_paintBar->setCurrentColor(paintScene()->color());
            updateInteractionCursor();
            return;
        }

        if (event->matches(QKeySequence::Undo)) {
            paintScene()->stopItemEditing();
            paintScene()->undoStack()->undo();
            return;
        }
        if (event->matches(QKeySequence::Redo)) {
            paintScene()->stopItemEditing();
            paintScene()->undoStack()->redo();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            if (m_options.enterKeyOutput) {
                emitOutputAction(m_options.enterAction, m_options.enterExit);
            }
            return;
        }
        if (event->matches(QKeySequence::Copy)) {
            if (!paintScene()->selectedItems().isEmpty()) {
                paintScene()->copySelected();
            } else {
                emit sigCommandRequested(CaptureCommand::kCopy, true);
            }
            return;
        }
        if (event->matches(QKeySequence::Paste)) {
            paintScene()->pasteCopied();
            return;
        }
        if (event->modifiers().testFlag(Qt::ControlModifier) && event->modifiers().testFlag(Qt::ShiftModifier)
            && event->key() == Qt::Key_S) {
            emit sigCommandRequested(CaptureCommand::kQuickSave, true);
            return;
        }
        if (event->matches(QKeySequence::Save)) {
            emit sigCommandRequested(CaptureCommand::kSave, true);
            return;
        }
        if (event->modifiers().testFlag(Qt::ControlModifier) && event->key() == Qt::Key_T) {
            emit sigCommandRequested(CaptureCommand::kPin, true);
            return;
        }
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            paintScene()->deleteSelected();
            return;
        }
    }
    PaintView::keyPressEvent(event);
}

void CaptureOverlay::focusOutEvent(QFocusEvent *event)
{
    if (m_options.abortOnFocusOut && m_phase != CapturePhase::kIdle && !isActiveWindow()) {
        emit sigCancelRequested();
        return;
    }
    PaintView::focusOutEvent(event);
}

void CaptureOverlay::updateHover()
{
    if (m_phase != CapturePhase::kSelecting || m_isDragging || !m_detectionEnabled || !m_options.detectWindows) {
        return;
    }

    const QPoint globalPosition = m_pendingHoverPosition + m_frame.logicalBounds.topLeft();
    m_hoverHierarchy = m_detector.detectHierarchyAt(globalPosition);
    if (m_hoverHierarchy.isEmpty()) {
        m_hoverRect = {};
        viewport()->update();
        return;
    }

    const int hover_index = m_options.detectUiElements ? m_hoverHierarchy.size() - 1 : 0;
    updateHoverHighlight(m_hoverHierarchy.at(hover_index));
}

void CaptureOverlay::updateHoverHighlight(const ElementCandidate &candidate)
{
    if (!candidate.isValid()) {
        m_hoverRect = {};
    } else {
        m_hoverRect = candidate.rect.translated(-m_frame.logicalBounds.topLeft());
    }
    viewport()->update();
}

void CaptureOverlay::drawSelection(QPainter &painter, const QRect &selection, bool hover)
{
    if (!selection.isValid()) {
        return;
    }
    if (m_options.hide1x1 && selection.width() == 1 && selection.height() == 1 && !hover) {
        return;
    }
    if (m_maskVisible && m_options.drawOutsideArea) {
        QPainterPath outside;
        outside.addRect(viewport()->rect());
        QPainterPath keep;
        keep.addRect(selection);
        painter.fillPath(outside.subtracted(keep), MASK_COLOR);
    }
    painter.setPen(QPen(hover ? HOVER_COLOR : BORDER_COLOR, hover ? 2 : qMax(1, m_options.borderWidth)));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(selection.adjusted(0, 0, -1, -1));
}

void CaptureOverlay::drawAnchors(QPainter &painter)
{
    const QRect selection = m_selectionState.current();
    if (!m_options.showAnchors || !selection.isValid()) {
        return;
    }
    painter.setPen(QPen(m_options.anchorBorderColor, 1));
    painter.setBrush(Qt::white);
    const QPoint points[] = {
        selection.topLeft(),
        QPoint(selection.center().x(), selection.top()),
        selection.topRight(),
        QPoint(selection.right(), selection.center().y()),
        selection.bottomRight(),
        QPoint(selection.center().x(), selection.bottom()),
        selection.bottomLeft(),
        QPoint(selection.left(), selection.center().y()),
    };
    for (const QPoint &point : points) {
        painter.drawRect(
            QRect(point.x() - ANCHOR_SIZE_PX / 2, point.y() - ANCHOR_SIZE_PX / 2, ANCHOR_SIZE_PX, ANCHOR_SIZE_PX));
    }
}

void CaptureOverlay::drawSizeLabel(QPainter &painter)
{
    const QRect selection = m_selectionState.current();
    if (!selection.isValid()) {
        return;
    }
    const QString text = QStringLiteral("%1 × %2").arg(selection.width()).arg(selection.height());
    QFont         font = painter.font();
    font.setPixelSize(12);
    painter.setFont(font);
    const QSize textSize = painter.fontMetrics().size(Qt::TextSingleLine, text);
    QRect       label(selection.left(),
                      selection.top() - textSize.height() - 10,
                      textSize.width() + 16,
                      textSize.height() + 8);
    if (label.top() < 0) {
        label.moveTop(selection.top() + 4);
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(20, 20, 20, 220));
    painter.drawRoundedRect(label, 4, 4);
    painter.setPen(Qt::white);
    painter.drawText(label, Qt::AlignCenter, text);
}

void CaptureOverlay::drawMagnifier(QPainter &painter)
{
    if (!m_magnifierVisible || m_backgroundPixmap.isNull()) {
        return;
    }
    m_magnifier.paint(painter, m_backgroundPixmap, m_frame.logicalComposite, m_pointerPosition);
}

void CaptureOverlay::drawTips(QPainter &painter)
{
    if (m_phase != CapturePhase::kSelecting) {
        return;
    }
    const QString text = QStringLiteral("拖动框选 · F 全屏 · Esc 取消");
    QRect         rect(12, 12, painter.fontMetrics().horizontalAdvance(text) + 20, 28);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 170));
    painter.drawRoundedRect(rect, 6, 6);
    painter.setPen(Qt::white);
    painter.drawText(rect, Qt::AlignCenter, text);
}

SelectionEdges CaptureOverlay::selectionEdgesAt(const QPoint &position) const
{
    const QRect selection = m_selectionState.current();
    if (!selection.isValid()
        || !selection.adjusted(-ANCHOR_HIT_PX, -ANCHOR_HIT_PX, ANCHOR_HIT_PX, ANCHOR_HIT_PX).contains(position)) {
        return SelectionEdge::kNone;
    }

    SelectionEdges edges;
    if (qAbs(position.x() - selection.left()) <= ANCHOR_HIT_PX) {
        edges |= SelectionEdge::kLeft;
    }
    if (qAbs(position.x() - selection.right()) <= ANCHOR_HIT_PX) {
        edges |= SelectionEdge::kRight;
    }
    if (qAbs(position.y() - selection.top()) <= ANCHOR_HIT_PX) {
        edges |= SelectionEdge::kTop;
    }
    if (qAbs(position.y() - selection.bottom()) <= ANCHOR_HIT_PX) {
        edges |= SelectionEdge::kBottom;
    }
    return edges;
}

SelectionEdges CaptureOverlay::selectionEdgesOutsideAt(const QPoint &position) const
{
    const QRect selection = m_selectionState.current();
    if (!selection.isValid() || selection.contains(position)) {
        return SelectionEdge::kNone;
    }

    SelectionEdges edges;
    if (position.x() < selection.left()) {
        edges |= SelectionEdge::kLeft;
    }
    if (position.x() > selection.right()) {
        edges |= SelectionEdge::kRight;
    }
    if (position.y() < selection.top()) {
        edges |= SelectionEdge::kTop;
    }
    if (position.y() > selection.bottom()) {
        edges |= SelectionEdge::kBottom;
    }
    return edges;
}

void CaptureOverlay::updateInteractionCursor()
{
    if (m_phase == CapturePhase::kSelecting) {
        viewport()->setCursor(Qt::CrossCursor);
        return;
    }
    if (m_phase != CapturePhase::kAnnotating) {
        viewport()->setCursor(Qt::ArrowCursor);
        return;
    }

    const QRect selection = m_selectionState.current();
    const PaintTool tool = paintScene()->tool();
    if (selection.contains(m_pointerPosition)) {
        switch (paintScene()->itemHandleAt(m_pointerPosition)) {
        case PaintItemHandle::kTopLeft:
        case PaintItemHandle::kBottomRight:
            viewport()->setCursor(Qt::SizeFDiagCursor);
            return;
        case PaintItemHandle::kTopRight:
        case PaintItemHandle::kBottomLeft:
            viewport()->setCursor(Qt::SizeBDiagCursor);
            return;
        case PaintItemHandle::kLeft:
        case PaintItemHandle::kRight:
            viewport()->setCursor(Qt::SizeHorCursor);
            return;
        case PaintItemHandle::kTop:
        case PaintItemHandle::kBottom:
            viewport()->setCursor(Qt::SizeVerCursor);
            return;
        case PaintItemHandle::kMove:
            viewport()->setCursor(Qt::SizeAllCursor);
            return;
        case PaintItemHandle::kNone:
            break;
        }
    }
    if (tool != PaintTool::kNone) {
        if (!selection.isValid() || !selection.contains(m_pointerPosition)) {
            viewport()->setCursor(Qt::ForbiddenCursor);
        } else {
            viewport()->setCursor(tool == PaintTool::kText ? Qt::IBeamCursor : Qt::CrossCursor);
        }
        return;
    }
    if (!selection.isValid()) {
        viewport()->setCursor(Qt::ArrowCursor);
        return;
    }

    if (selection.contains(m_pointerPosition)) {
        const SelectionEdges edge = selectionEdgesAt(m_pointerPosition);
        if (edge != SelectionEdge::kNone) {
            const bool left = edge.testFlag(SelectionEdge::kLeft);
            const bool right = edge.testFlag(SelectionEdge::kRight);
            const bool top = edge.testFlag(SelectionEdge::kTop);
            const bool bottom = edge.testFlag(SelectionEdge::kBottom);
            if ((left || right) && (top || bottom)) {
                viewport()->setCursor((left && top) || (right && bottom) ? Qt::SizeFDiagCursor
                                                                        : Qt::SizeBDiagCursor);
            } else if (left || right) {
                viewport()->setCursor(Qt::SizeHorCursor);
            } else {
                viewport()->setCursor(Qt::SizeVerCursor);
            }
            return;
        }
        viewport()->setCursor(Qt::SizeAllCursor);
        return;
    }

    const SelectionEdges outside_edges = selectionEdgesOutsideAt(m_pointerPosition);
    const bool left = outside_edges.testFlag(SelectionEdge::kLeft);
    const bool right = outside_edges.testFlag(SelectionEdge::kRight);
    const bool top = outside_edges.testFlag(SelectionEdge::kTop);
    const bool bottom = outside_edges.testFlag(SelectionEdge::kBottom);
    if ((left || right) && (top || bottom)) {
        viewport()->setCursor((left && top) || (right && bottom) ? Qt::SizeFDiagCursor
                                                                : Qt::SizeBDiagCursor);
    } else if (left || right) {
        viewport()->setCursor(Qt::SizeHorCursor);
    } else if (top || bottom) {
        viewport()->setCursor(Qt::SizeVerCursor);
    } else {
        viewport()->setCursor(Qt::ArrowCursor);
    }
}

void CaptureOverlay::applyStartMode()
{
    const QRect full(QPoint(0, 0), m_frame.logicalComposite.size());
    switch (m_request.startMode) {
    case CaptureStartMode::kFullDesktop:
        m_selectionState.setSelection(full);
        break;
    case CaptureStartMode::kLastArea:
        if (!m_history.current().isNull()) {
            m_selectionState.setSelection(m_history.current());
        }
        break;
    case CaptureStartMode::kCenteredSize: {
        if (m_request.centeredSize.isValid()) {
            QRect centered(QPoint(0, 0), m_request.centeredSize);
            centered.moveCenter(QCursor::pos() - m_frame.logicalBounds.topLeft());
            m_selectionState.setSelection(centered);
        }
        break;
    }
    case CaptureStartMode::kActiveScreen: {
        const QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
        if (screen != nullptr) {
            m_selectionState.setSelection(screen->geometry().translated(-m_frame.logicalBounds.topLeft()));
        }
        break;
    }
    case CaptureStartMode::kTwoCorners:
        if (m_request.initialSelection.isValid()) {
            m_selectionState.setSelection(m_request.initialSelection.translated(-m_frame.logicalBounds.topLeft()));
        }
        break;
    case CaptureStartMode::kPoint:
        break;
    }
    viewport()->update();
}

void CaptureOverlay::updatePaintBarPosition()
{
    if (m_paintBar == nullptr || m_phase != CapturePhase::kAnnotating) {
        return;
    }

    const QRect selection = m_selectionState.current();
    const QSize barSize = m_paintBar->size();
    int         x = selection.left();
    int         y = selection.bottom() + 8;
    if (y + barSize.height() > height()) {
        y = selection.top() - barSize.height() - 8;
    }
    x = qBound(0, x, qMax(0, width() - barSize.width()));
    y = qBound(0, y, qMax(0, height() - barSize.height()));
    m_paintBar->move(x, y);
}

void CaptureOverlay::emitOutputAction(CaptureOutputAction action, bool closeAfter)
{
    switch (action) {
    case CaptureOutputAction::kCopy:
        emit sigCommandRequested(CaptureCommand::kCopy, closeAfter);
        break;
    case CaptureOutputAction::kPin:
        emit sigCommandRequested(CaptureCommand::kPin, closeAfter);
        break;
    case CaptureOutputAction::kSave:
        emit sigCommandRequested(CaptureCommand::kSave, closeAfter);
        break;
    case CaptureOutputAction::kQuickSave:
        emit sigCommandRequested(CaptureCommand::kQuickSave, closeAfter);
        break;
    case CaptureOutputAction::kNone:
        break;
    }
}
