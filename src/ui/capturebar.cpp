#include "capturebar.h"

#include "ui/glyphicon.h"

#include <QBoxLayout>
#include <QCursor>
#include <QEasingCurve>
#include <QGuiApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScreen>
#include <QToolButton>

#include <windows.h>

namespace {
QPoint clampedTopLeft(const QRect &screen_rect, const QSize &bar_size, const QPoint &top_left)
{
    const int max_x = qMax(screen_rect.left(), screen_rect.right() - bar_size.width() + 1);
    const int max_y = qMax(screen_rect.top(), screen_rect.bottom() - bar_size.height() + 1);
    return QPoint(qBound(screen_rect.left(), top_left.x(), max_x), qBound(screen_rect.top(), top_left.y(), max_y));
}

class DockHotspot final : public QWidget
{
public:
    explicit DockHotspot(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 90));
        painter.drawRoundedRect(rect().adjusted(1, 1, 1, 1), 2, 2);
        painter.setBrush(QColor(0xD9, 0x4E, 0x3E));
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 2, 2);
    }
};
} // namespace

CaptureBar::CaptureBar(QWidget *parent)
    : QWidget(parent)
{
    qRegisterMetaType<ApplicationAction>("ApplicationAction");

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint
                   | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setFocusPolicy(Qt::NoFocus);
    setStyleSheet(QStringLiteral("QToolButton {"
                                 "  color: #E8E8E8;"
                                 "  background: transparent;"
                                 "  border: none;"
                                 "  border-radius: 5px;"
                                 "  font-size: 13px;"
                                 "}"
                                 "QToolButton:hover { background: #3C3C3C; }"
                                 "QToolButton:pressed { background: #0A84FF; }"
                                 "QToolButton:disabled { color: #777777; }"));

    m_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(2);

    m_dragHandle = new QWidget(this);
    m_dragHandle->setObjectName(QStringLiteral("dragHandle_CaptureBar"));
    m_dragHandle->setCursor(Qt::SizeAllCursor);
    m_dragHandle->setStyleSheet(QStringLiteral("background-color: rgba(255, 255, 255, 35);"
                                               "border-radius: 2px;"));
    m_dragHandle->installEventFilter(this);
    m_layout->addWidget(m_dragHandle, 0, Qt::AlignCenter);

    m_hotspot = new DockHotspot(this);
    m_hotspot->setObjectName(QStringLiteral("hotspot_CaptureBar"));
    m_hotspot->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint
                              | Qt::WindowDoesNotAcceptFocus);
    m_hotspot->setAttribute(Qt::WA_ShowWithoutActivating);
    m_hotspot->setAttribute(Qt::WA_TranslucentBackground);
    m_hotspot->setCursor(Qt::PointingHandCursor);
    m_hotspot->installEventFilter(this);
    m_hotspot->hide();

    m_autoHideTimer.setSingleShot(true);
    m_autoHideTimer.setInterval(m_hideDelayMs);
    connect(&m_autoHideTimer, &QTimer::timeout, this, &CaptureBar::hideToHotspot);
    m_revealTimer.setSingleShot(true);
    m_revealTimer.setInterval(m_revealDelayMs);
    connect(&m_revealTimer, &QTimer::timeout, this, &CaptureBar::startRevealAnimation);
    m_pointerTimer.setInterval(25);
    connect(&m_pointerTimer, &QTimer::timeout, this, &CaptureBar::updatePointerState);

    m_slideAnimation = new QPropertyAnimation(this, "pos", this);
    m_slideAnimation->setDuration(m_slideDurationMs);
    m_slideAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_slideAnimation, &QPropertyAnimation::finished, this, &CaptureBar::finishSlideAnimation);

    for (const ApplicationAction action : captureBarActions()) {
        m_layout->addWidget(createActionButton(action), 0, Qt::AlignCenter);
    }

    updateOrientation();
    winId();
    m_hotspot->winId();
    applyNoActivateStyle();
}

CaptureBar::~CaptureBar() = default;

void CaptureBar::setDockEdge(DockEdge edge)
{
    if (m_dockEdge == edge) {
        return;
    }

    m_dockEdge = edge;
    updateOrientation();
}

DockEdge CaptureBar::dockEdge() const
{
    return m_dockEdge;
}

void CaptureBar::setFreeOrientation(bool horizontal)
{
    m_isHorizontalLayout = horizontal;
    if (m_dockEdge == DockEdge::None) {
        updateOrientation();
    }
}

bool CaptureBar::isHorizontalLayout() const
{
    return m_isHorizontalLayout;
}

void CaptureBar::setAutoHideEnabled(bool enabled)
{
    m_autoHideEnabled = enabled;
    if (!enabled) {
        m_autoHideTimer.stop();
        m_revealTimer.stop();
        m_pointerTimer.stop();
        m_slideAnimation->stop();
        m_hotspot->hide();
        if (m_dockEdge != DockEdge::None) {
            restoreVisible();
        }
    } else if (m_dockEdge != DockEdge::None && isVisible()) {
        m_pointerTimer.start();
    }
}

bool CaptureBar::isAutoHideEnabled() const
{
    return m_autoHideEnabled;
}

void CaptureBar::setSnapDistancePx(int distancePx)
{
    m_snapDistancePx = qBound(0, distancePx, 48);
}

void CaptureBar::setHotspotWidthPx(int widthPx)
{
    m_hotspotWidthPx = qBound(2, widthPx, 12);
}

void CaptureBar::setHotspotLengthPx(int lengthPx)
{
    m_hotspotLengthPx = qBound(20, lengthPx, 320);
}

void CaptureBar::setHideDelayMs(int delayMs)
{
    m_hideDelayMs = qBound(100, delayMs, 3000);
    m_autoHideTimer.setInterval(m_hideDelayMs);
}

void CaptureBar::setRevealDelayMs(int delayMs)
{
    m_revealDelayMs = qBound(0, delayMs, 1000);
    m_revealTimer.setInterval(m_revealDelayMs);
}

void CaptureBar::setSlideDurationMs(int durationMs)
{
    m_slideDurationMs = qBound(0, durationMs, 1000);
    m_slideAnimation->setDuration(m_slideDurationMs);
}

void CaptureBar::showAtDock(
    DockEdge edge, qreal positionRatio, const QString &screenName, const QPoint &freeTopLeft, bool useFreePosition)
{
    m_autoHideTimer.stop();
    m_revealTimer.stop();
    m_pointerTimer.stop();
    m_slideAnimation->stop();

    setDockEdge(edge);
    m_positionRatio = qBound(0.0, positionRatio, 1.0);

    QScreen *screen = findScreen(screenName);
    if (!screen) {
        screen = QGuiApplication::screenAt(QCursor::pos());
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    m_screenRect = screen->availableGeometry();
    m_screenName = screen->name();
    QRect barRect;
    if (edge == DockEdge::None && useFreePosition) {
        barRect = QRect(freeTopLeft, size());
        m_freeTopLeft = freeTopLeft;
        m_hasFreePosition = true;
    } else {
        barRect = placeAlongDock(m_screenRect, size(), edge, m_positionRatio);
    }
    barRect.moveTopLeft(clampedTopLeft(m_screenRect, size(), barRect.topLeft()));
    m_visibleRect = barRect;
    setGeometry(barRect);
    restoreVisible();
    m_isHiddenToHotspot = false;
    m_hotspot->hide();
    show();
    raise();
    applyNoActivateStyle();
    if (m_autoHideEnabled && edge != DockEdge::None) {
        m_pointerTimer.start();
        if (!frameGeometry().contains(QCursor::pos())) {
            scheduleAutoHide();
        }
    }
}

void CaptureBar::scheduleAutoHide()
{
    if (!m_autoHideEnabled || m_isSuppressed || m_isDragging || m_dockEdge == DockEdge::None
        || m_autoHideState != AutoHideState::kVisible) {
        return;
    }
    m_autoHideState = AutoHideState::kHidePending;
    m_autoHideTimer.start(m_hideDelayMs);
}

void CaptureBar::suppressForCapture()
{
    m_isSuppressed = true;
    m_autoHideTimer.stop();
    m_revealTimer.stop();
    m_pointerTimer.stop();
    m_slideAnimation->stop();
    m_autoHideState = AutoHideState::kVisible;
    m_hotspot->hide();
    hide();
}

void CaptureBar::restoreAfterCapture()
{
    m_isSuppressed = false;
    showAtDock(m_dockEdge,
               m_positionRatio,
               m_screenName,
               m_freeTopLeft,
               m_dockEdge == DockEdge::None && m_hasFreePosition);
}

void CaptureBar::hideCompletely()
{
    m_autoHideTimer.stop();
    m_revealTimer.stop();
    m_pointerTimer.stop();
    m_slideAnimation->stop();
    m_autoHideState = AutoHideState::kVisible;
    m_hotspot->hide();
    hide();
}

bool CaptureBar::isHiddenToHotspot() const
{
    return m_isHiddenToHotspot;
}

void CaptureBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(86, 86, 86), 1));
    painter.setBrush(QColor(43, 43, 43, 235));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 8, 8);
}

bool CaptureBar::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_dragHandle && event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            m_isDragging = true;
            m_autoHideTimer.stop();
            m_revealTimer.stop();
            m_slideAnimation->stop();
            m_dragOffset = mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
            return true;
        }
    }
    if (watched == m_dragHandle && event->type() == QEvent::MouseMove && m_isDragging) {
        const auto  *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint global_position = mouseEvent->globalPosition().toPoint();
        if (QScreen *screen = screenForPointOrWindow(global_position)) {
            m_screenRect = screen->availableGeometry();
            m_screenName = screen->name();
        }
        const QPoint desired = global_position - m_dragOffset;
        move(clampedTopLeft(m_screenRect, size(), desired));
        return true;
    }
    if (watched == m_dragHandle && event->type() == QEvent::MouseButtonRelease && m_isDragging) {
        const auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            finishDrag(mouseEvent->globalPosition().toPoint());
            return true;
        }
    }
    if (watched == m_hotspot && event->type() == QEvent::Enter) {
        revealFromHotspot();
        return true;
    }
    if (watched == m_hotspot && event->type() == QEvent::Leave) {
        if (m_autoHideState == AutoHideState::kRevealPending) {
            m_revealTimer.stop();
            m_autoHideState = AutoHideState::kHidden;
        }
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void CaptureBar::enterEvent(QEnterEvent *event)
{
    m_autoHideTimer.stop();
    if (m_autoHideState == AutoHideState::kHidePending || m_autoHideState == AutoHideState::kHiding) {
        cancelHideAnimation();
    }
    QWidget::enterEvent(event);
}

void CaptureBar::leaveEvent(QEvent *event)
{
    scheduleAutoHide();
    QWidget::leaveEvent(event);
}

QToolButton *CaptureBar::createActionButton(ApplicationAction action)
{
    auto *button = new QToolButton(this);
    button->setObjectName(actionObjectName(action));
    button->setIcon(createGlyphIcon(applicationActionGlyph(action)));
    button->setIconSize(QSize(20, 20));
    button->setFixedSize(36, 36);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setEnabled(isImplementedAction(action));
    button->setToolTip(isImplementedAction(action) ? actionDisplayName(action)
                                                   : QStringLiteral("%1（功能开发中）").arg(actionDisplayName(action)));
    if (action == ApplicationAction::kOcrSnip) {
        m_ocrButton = button;
    }

    connect(button, &QToolButton::clicked, this, [this, action]() { emit sigActionTriggered(action); });
    return button;
}

void CaptureBar::setOcrAvailable(bool available)
{
    if (m_ocrButton == nullptr) {
        return;
    }
    m_ocrButton->setEnabled(available);
    m_ocrButton->setToolTip(available ? actionDisplayName(ApplicationAction::kOcrSnip)
                                      : QStringLiteral("OCR（未安装 Windows OCR 语言包）"));
}

void CaptureBar::updateOrientation()
{
    if (m_dockEdge == DockEdge::Top || m_dockEdge == DockEdge::Bottom) {
        m_isHorizontalLayout = true;
    } else if (m_dockEdge == DockEdge::Left || m_dockEdge == DockEdge::Right) {
        m_isHorizontalLayout = false;
    }
    m_layout->setDirection(m_isHorizontalLayout ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    m_dragHandle->setFixedSize(m_isHorizontalLayout ? QSize(12, 36) : QSize(36, 12));
    m_layout->invalidate();
    adjustSize();
    setFixedSize(sizeHint());
}

void CaptureBar::hideToHotspot()
{
    if (!m_autoHideEnabled || m_isSuppressed || m_isDragging || m_dockEdge == DockEdge::None) {
        return;
    }
    if (m_autoHideState == AutoHideState::kHidden || m_autoHideState == AutoHideState::kHiding
        || m_autoHideState == AutoHideState::kRevealing) {
        return;
    }
    if (frameGeometry().contains(QCursor::pos())) {
        m_autoHideState = AutoHideState::kVisible;
        return;
    }

    m_visibleRect = geometry();
    m_positionRatio = positionRatioFor(m_visibleRect, m_screenRect);
    m_hotspotRect = calculateDockHotspot(m_screenRect, m_visibleRect, m_dockEdge, m_hotspotWidthPx, m_hotspotLengthPx);
    m_autoHideState = AutoHideState::kHiding;
    m_slideAnimation->stop();
    m_slideAnimation->setStartValue(m_visibleRect.topLeft());
    m_slideAnimation->setEndValue(hiddenTopLeft());
    m_slideAnimation->start();
}

void CaptureBar::revealFromHotspot()
{
    if (m_autoHideState != AutoHideState::kHidden || !m_hotspotRect.contains(QCursor::pos())) {
        return;
    }

    m_autoHideState = AutoHideState::kRevealPending;
    m_revealTimer.start(m_revealDelayMs);
}

void CaptureBar::startRevealAnimation()
{
    if (m_autoHideState != AutoHideState::kRevealPending || m_isSuppressed) {
        return;
    }
    if (!m_hotspotRect.contains(QCursor::pos())) {
        m_autoHideState = AutoHideState::kHidden;
        return;
    }

    m_hotspot->hide();
    QRect reveal_rect = m_visibleRect;
    reveal_rect.moveTopLeft(hiddenTopLeft());
    setGeometry(reveal_rect);
    m_isHiddenToHotspot = false;
    show();
    raise();
    applyNoActivateStyle();
    m_autoHideState = AutoHideState::kRevealing;
    m_slideAnimation->stop();
    m_slideAnimation->setStartValue(reveal_rect.topLeft());
    m_slideAnimation->setEndValue(m_visibleRect.topLeft());
    m_slideAnimation->start();
}

void CaptureBar::finishSlideAnimation()
{
    if (m_autoHideState == AutoHideState::kHiding) {
        m_isHiddenToHotspot = true;
        hide();
        setGeometry(m_visibleRect);
        m_hotspot->setGeometry(m_hotspotRect);
        m_hotspot->show();
        m_hotspot->raise();
        m_autoHideState = AutoHideState::kHidden;
        applyNoActivateStyle();
        m_pointerTimer.start();
        return;
    }

    if (m_autoHideState == AutoHideState::kRevealing) {
        setGeometry(m_visibleRect);
        m_autoHideState = AutoHideState::kVisible;
        m_isHiddenToHotspot = false;
        m_pointerTimer.start();
        if (!frameGeometry().contains(QCursor::pos())) {
            scheduleAutoHide();
        }
    }
}

void CaptureBar::cancelHideAnimation()
{
    if (m_autoHideState != AutoHideState::kHidePending && m_autoHideState != AutoHideState::kHiding) {
        return;
    }
    m_autoHideTimer.stop();
    m_slideAnimation->stop();
    m_autoHideState = AutoHideState::kVisible;
    m_isHiddenToHotspot = false;
    setGeometry(m_visibleRect);
    show();
    raise();
    applyNoActivateStyle();
}

void CaptureBar::restoreVisible()
{
    if (m_isSuppressed || m_dockEdge == DockEdge::None || !m_visibleRect.isValid()) {
        return;
    }
    m_autoHideTimer.stop();
    m_revealTimer.stop();
    m_slideAnimation->stop();
    m_hotspot->hide();
    m_autoHideState = AutoHideState::kVisible;
    m_isHiddenToHotspot = false;
    setGeometry(m_visibleRect);
    show();
    raise();
    applyNoActivateStyle();
    if (m_autoHideEnabled) {
        m_pointerTimer.start();
        if (!frameGeometry().contains(QCursor::pos())) {
            scheduleAutoHide();
        }
    }
}

void CaptureBar::updatePointerState()
{
    if (!m_autoHideEnabled || m_isSuppressed || m_isDragging || m_dockEdge == DockEdge::None) {
        return;
    }

    const QPoint cursor = QCursor::pos();
    if (m_autoHideState == AutoHideState::kHidden) {
        if (m_hotspotRect.contains(cursor)) {
            revealFromHotspot();
        }
        return;
    }
    if (m_autoHideState == AutoHideState::kRevealPending) {
        if (!m_hotspotRect.contains(cursor)) {
            m_revealTimer.stop();
            m_autoHideState = AutoHideState::kHidden;
        }
        return;
    }
    if (m_autoHideState == AutoHideState::kHidePending || m_autoHideState == AutoHideState::kHiding) {
        if (frameGeometry().contains(cursor) || m_visibleRect.contains(cursor)) {
            cancelHideAnimation();
        }
        return;
    }
    if (m_autoHideState == AutoHideState::kVisible) {
        if (frameGeometry().contains(cursor)) {
            m_autoHideTimer.stop();
            m_autoHideState = AutoHideState::kVisible;
        } else if (!m_autoHideTimer.isActive()) {
            scheduleAutoHide();
        }
    }
}

void CaptureBar::finishDrag(const QPoint &globalPosition)
{
    m_isDragging = false;
    QScreen *screen = screenForPointOrWindow(globalPosition);
    if (!screen) {
        return;
    }

    m_screenRect = screen->availableGeometry();
    m_screenName = screen->name();
    const DockPlacement placement
        = calculateDockPlacement(m_screenRect, size(), pos(), m_snapDistancePx, m_hotspotWidthPx, m_hotspotLengthPx);
    setDockEdge(placement.edge);
    setGeometry(placement.barRect);
    m_visibleRect = placement.barRect;
    m_hotspotRect = placement.hotspotRect;
    m_freeTopLeft = placement.barRect.topLeft();
    m_hasFreePosition = true;
    m_positionRatio = positionRatioFor(placement.barRect, m_screenRect);
    m_autoHideState = AutoHideState::kVisible;
    m_isHiddenToHotspot = false;
    m_hotspot->hide();
    show();
    raise();
    applyNoActivateStyle();
    emit sigPlacementChanged(m_dockEdge,
                             m_positionRatio,
                             m_screenName,
                             m_freeTopLeft,
                             m_hasFreePosition,
                             m_isHorizontalLayout);
    if (m_autoHideEnabled && m_dockEdge != DockEdge::None) {
        m_pointerTimer.start();
        if (!frameGeometry().contains(QCursor::pos())) {
            scheduleAutoHide();
        }
    } else {
        m_pointerTimer.stop();
    }
}

QScreen *CaptureBar::screenForPointOrWindow(const QPoint &globalPosition) const
{
    if (QScreen *screen = QGuiApplication::screenAt(globalPosition)) {
        return screen;
    }

    QScreen *best_screen = nullptr;
    int      best_area = 0;
    for (QScreen *screen : QGuiApplication::screens()) {
        const QRect intersection = frameGeometry().intersected(screen->availableGeometry());
        const int   area = intersection.width() * intersection.height();
        if (area > best_area) {
            best_area = area;
            best_screen = screen;
        }
    }
    return best_screen != nullptr ? best_screen : QGuiApplication::primaryScreen();
}

QPoint CaptureBar::hiddenTopLeft() const
{
    switch (m_dockEdge) {
    case DockEdge::Top:
        return QPoint(m_visibleRect.left(), m_screenRect.top() - m_visibleRect.height() - 2);
    case DockEdge::Bottom:
        return QPoint(m_visibleRect.left(), m_screenRect.bottom() + 2);
    case DockEdge::Left:
        return QPoint(m_screenRect.left() - m_visibleRect.width() - 2, m_visibleRect.top());
    case DockEdge::Right:
        return QPoint(m_screenRect.right() + 2, m_visibleRect.top());
    case DockEdge::None:
        break;
    }
    return m_visibleRect.topLeft();
}

qreal CaptureBar::positionRatioFor(const QRect &barRect, const QRect &screenRect) const
{
    if (m_dockEdge == DockEdge::Left || m_dockEdge == DockEdge::Right) {
        const int available = screenRect.height() - barRect.height();
        return available > 0 ? qreal(barRect.top() - screenRect.top()) / available : 0.5;
    }
    if (m_dockEdge == DockEdge::Top || m_dockEdge == DockEdge::Bottom) {
        const int available = screenRect.width() - barRect.width();
        return available > 0 ? qreal(barRect.left() - screenRect.left()) / available : 0.5;
    }
    return 0.5;
}

QScreen *CaptureBar::findScreen(const QString &screenName) const
{
    if (screenName.isEmpty()) {
        return nullptr;
    }

    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (screen->name() == screenName) {
            return screen;
        }
    }
    return nullptr;
}

void CaptureBar::applyNoActivateStyle()
{
    const HWND     window = reinterpret_cast<HWND>(winId());
    const LONG_PTR style = GetWindowLongPtrW(window, GWL_EXSTYLE);
    SetWindowLongPtrW(window, GWL_EXSTYLE, style | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);

    const HWND     hotspot = reinterpret_cast<HWND>(m_hotspot->winId());
    const LONG_PTR hotspotStyle = GetWindowLongPtrW(hotspot, GWL_EXSTYLE);
    SetWindowLongPtrW(hotspot, GWL_EXSTYLE, hotspotStyle | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);
}
