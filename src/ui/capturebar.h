#pragma once

#include "app/applicationaction.h"
#include "ui/capturebargeometry.h"

#include <QTimer>
#include <QWidget>

class QBoxLayout;
class QPropertyAnimation;
class QScreen;
class QToolButton;

class CaptureBar : public QWidget
{
    Q_OBJECT

public:
    explicit CaptureBar(QWidget *parent = nullptr);
    ~CaptureBar() override;

    void     setDockEdge(DockEdge edge);
    DockEdge dockEdge() const;
    void     setAutoHideEnabled(bool enabled);
    bool     isAutoHideEnabled() const;
    void     setSnapDistancePx(int distancePx);
    void     setHotspotWidthPx(int widthPx);
    void     setHotspotLengthPx(int lengthPx);
    void     setHideDelayMs(int delayMs);
    void     setRevealDelayMs(int delayMs);
    void     setSlideDurationMs(int durationMs);
    void     setOcrAvailable(bool available);
    void     showAtDock(DockEdge       edge,
                        qreal          positionRatio,
                        const QString &screenName = {},
                        const QPoint  &freeTopLeft = {},
                        bool           useFreePosition = false);
    void     setFreeOrientation(bool horizontal);
    bool     isHorizontalLayout() const;
    void     scheduleAutoHide();
    void     suppressForCapture();
    void     restoreAfterCapture();
    void     hideCompletely();
    bool     isHiddenToHotspot() const;

signals:
    void sigActionTriggered(ApplicationAction action);
    void sigPlacementChanged(DockEdge       edge,
                             qreal          positionRatio,
                             const QString &screenName,
                             const QPoint  &freeTopLeft,
                             bool           hasFreePosition,
                             bool           horizontal);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    enum class AutoHideState { kVisible, kHidePending, kHiding, kHidden, kRevealPending, kRevealing };

    QToolButton *createActionButton(ApplicationAction action);
    void         updateOrientation();
    void         applyNoActivateStyle();
    void         hideToHotspot();
    void         revealFromHotspot();
    void         startRevealAnimation();
    void         finishSlideAnimation();
    void         cancelHideAnimation();
    void         restoreVisible();
    void         updatePointerState();
    void         finishDrag(const QPoint &globalPosition);
    qreal        positionRatioFor(const QRect &barRect, const QRect &screenRect) const;
    QScreen     *findScreen(const QString &screenName) const;
    QScreen     *screenForPointOrWindow(const QPoint &globalPosition) const;
    QPoint       hiddenTopLeft() const;

    QBoxLayout         *m_layout{nullptr};
    QWidget            *m_dragHandle{nullptr};
    QWidget            *m_hotspot{nullptr};
    QPropertyAnimation *m_slideAnimation{nullptr};
    QToolButton        *m_ocrButton{nullptr};
    QTimer              m_autoHideTimer;
    QTimer              m_revealTimer;
    QTimer              m_pointerTimer;
    QPoint              m_dragOffset;
    QRect               m_screenRect;
    QRect               m_visibleRect;
    QRect               m_hotspotRect;
    QPoint              m_freeTopLeft;
    QString             m_screenName;
    qreal               m_positionRatio{0.5};
    DockEdge            m_dockEdge{DockEdge::Right};
    AutoHideState       m_autoHideState{AutoHideState::kVisible};
    int                 m_snapDistancePx{12};
    int                 m_hotspotWidthPx{4};
    int                 m_hotspotLengthPx{50};
    int                 m_hideDelayMs{850};
    int                 m_revealDelayMs{200};
    int                 m_slideDurationMs{300};
    bool                m_autoHideEnabled{true};
    bool                m_hasFreePosition{false};
    bool                m_isHorizontalLayout{false};
    bool                m_isHiddenToHotspot{false};
    bool                m_isDragging{false};
    bool                m_isSuppressed{false};
};
