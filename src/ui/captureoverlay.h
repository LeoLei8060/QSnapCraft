#pragma once

#include "capture/captureoptions.h"
#include "capture/capturetypes.h"
#include "capture/selectionstate.h"
#include "paint/paintview.h"
#include "platform/windows/elementdetector.h"
#include "ui/magnifier.h"

#include <QPixmap>
#include <QTimer>

class PaintBar;

class CaptureOverlay : public PaintView
{
    Q_OBJECT

public:
    explicit CaptureOverlay(QWidget *parent = nullptr);
    ~CaptureOverlay() override;

    void         begin(const CaptureFrame &frame, const CaptureOptions &options, CaptureRequest request);
    void         setPhase(CapturePhase phase);
    CapturePhase phase() const;
    void         setSelection(const QRect &selection);
    QRect        currentSelection() const;
    void         enterAnnotating();
    void         recordSelectionForHistory();
    QImage       renderSelection() const;
    QString      selectionSizeLabel() const;
    bool         capturedCursorEnabled() const;
    void         setCapturedCursorEnabled(bool enabled);

signals:
    void sigSelectionConfirmed(const QRect &selection);
    void sigCancelRequested();
    void sigCommandRequested(CaptureCommand command, bool closeAfter);

protected:
    bool event(QEvent *event) override;
    void drawOverlay(QPainter &painter) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void           updateHover();
    void           updateHoverHighlight(const ElementCandidate &candidate);
    void           drawSelection(QPainter &painter, const QRect &selection, bool hover);
    void           drawAnchors(QPainter &painter);
    void           drawSizeLabel(QPainter &painter);
    void           drawMagnifier(QPainter &painter);
    void           drawTips(QPainter &painter);
    void           updateCapturedCursorPreview();
    SelectionEdges selectionEdgesAt(const QPoint &position) const;
    SelectionEdges selectionEdgesOutsideAt(const QPoint &position) const;
    void           updateInteractionCursor();
    void           applyStartMode();
    void           updatePaintBarPosition();
    void           emitOutputAction(CaptureOutputAction action, bool closeAfter);

    CaptureFrame     m_frame;
    CaptureOptions   m_options;
    CaptureRequest   m_request;
    CapturePhase     m_phase{CapturePhase::kIdle};
    SelectionState   m_selectionState;
    SelectionHistory m_history;
    ElementDetector  m_detector;
    Magnifier        m_magnifier;
    PaintBar        *m_paintBar{nullptr};
    QPixmap          m_backgroundPixmap;
    QTimer           m_hoverTimer;

    QVector<ElementCandidate> m_hoverHierarchy;
    QRect                     m_hoverRect;
    QPoint                    m_pendingHoverPosition;
    QPoint                    m_anchor;
    QPoint                    m_pointerPosition;
    SelectionEdges            m_activeEdges;
    bool                      m_isDragging{false};
    bool                      m_adopting{false};
    bool                      m_dragged{false};
    bool                      m_isResizing{false};
    bool                      m_isMoving{false};
    bool                      m_detectionEnabled{true};
    bool                      m_maskVisible{true};
    bool                      m_magnifierVisible{true};
    bool                      m_physicalSizeUnit{false};
};
