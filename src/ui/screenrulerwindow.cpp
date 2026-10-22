#include "screenrulerwindow.h"

#include "capture/capturetypes.h"
#include "platform/windows/desktopcapture.h"
#include "ui/glyphicon.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QContextMenuEvent>
#include <QCursor>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <functional>

namespace {
constexpr int RULER_THICKNESS = 100;
constexpr int TICK_ZONE = 20;
constexpr int RESIZE_ZONE = 10;
constexpr int MIN_VERTICAL_LENGTH = 200;
constexpr int MIN_HORIZONTAL_LENGTH = 250;

QPoint clampedPosition(const QRect &screen_rect, const QRect &geometry)
{
    const int max_x = qMax(screen_rect.left(), screen_rect.right() - geometry.width() + 1);
    const int max_y = qMax(screen_rect.top(), screen_rect.bottom() - geometry.height() + 1);
    return QPoint(qBound(screen_rect.left(), geometry.left(), max_x), qBound(screen_rect.top(), geometry.top(), max_y));
}

class RulerMeasurementOverlay final : public QWidget
{
public:
    using FinishCallback = std::function<void(const QString &)>;
    using CancelCallback = std::function<void()>;

    RulerMeasurementOverlay(CaptureFrame   frame,
                            bool           is_rectangle,
                            RulerUnit      unit,
                            int            ppi_tenths,
                            FinishCallback finish,
                            CancelCallback cancel,
                            QWidget       *parent)
        : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint)
        , m_frame(std::move(frame))
        , m_isRectangle(is_rectangle)
        , m_unit(unit)
        , m_ppiTenths(ppi_tenths)
        , m_finish(std::move(finish))
        , m_cancel(std::move(cancel))
    {
        setObjectName(QStringLiteral("rulerMeasurementOverlay"));
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::CrossCursor);
        setGeometry(m_frame.logicalBounds);
        m_start = QCursor::pos() - m_frame.logicalBounds.topLeft();
        m_current = m_start;
    }

    void begin()
    {
        show();
        raise();
        activateWindow();
        setFocus(Qt::OtherFocusReason);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.drawImage(QPoint(0, 0), m_frame.logicalComposite);
        painter.fillRect(rect(), QColor(0, 0, 0, 18));
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(0, 190, 210), 1));
        painter.drawLine(0, m_current.y(), width(), m_current.y());
        painter.drawLine(m_current.x(), 0, m_current.x(), height());

        if (m_hasOrigin) {
            painter.setPen(QPen(QColor(0, 190, 210), 2));
            if (m_isRectangle) {
                painter.drawRect(QRect(m_start, m_current).normalized());
            } else {
                painter.drawLine(m_start, m_current);
                painter.drawEllipse(m_start, 3, 3);
                painter.drawEllipse(m_current, 3, 3);
            }
            const QString text = measurementText();
            QFont         font = painter.font();
            font.setBold(true);
            font.setPixelSize(13);
            painter.setFont(font);
            const QSize text_size = painter.fontMetrics().size(Qt::TextSingleLine, text);
            QRect       label(m_current + QPoint(14, 14), text_size + QSize(14, 8));
            if (label.right() >= width()) {
                label.moveRight(m_current.x() - 14);
            }
            if (label.bottom() >= height()) {
                label.moveBottom(m_current.y() - 14);
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 255, 220, 235));
            painter.drawRoundedRect(label, 3, 3);
            painter.setPen(QColor(20, 20, 20));
            painter.drawText(label, Qt::AlignCenter, text);
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::RightButton) {
            cancel();
            return;
        }
        if (event->button() != Qt::LeftButton) {
            return;
        }
        m_current = event->position().toPoint();
        if (m_isWaitingSecondClick) {
            finish();
            return;
        }
        m_start = m_current;
        m_hasOrigin = true;
        m_dragging = true;
        update();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        m_current = event->position().toPoint();
        if (!m_isRectangle && m_hasOrigin && event->modifiers().testFlag(Qt::ControlModifier)) {
            const QPoint delta = m_current - m_start;
            if (qAbs(delta.x()) > qAbs(delta.y())) {
                m_current.setY(m_start.y());
            } else {
                m_current.setX(m_start.x());
            }
        }
        update();
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton || !m_dragging) {
            return;
        }
        m_current = event->position().toPoint();
        m_dragging = false;
        if (m_current == m_start) {
            m_isWaitingSecondClick = true;
            return;
        }
        finish();
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape) {
            cancel();
            return;
        }
        QWidget::keyPressEvent(event);
    }

private:
    QString measurementText() const
    {
        if (m_isRectangle) {
            return formatRulerMeasurement(rulerSpanPixels(m_start.x(), m_current.x()), m_unit, m_ppiTenths)
                   + QStringLiteral(" x ")
                   + formatRulerMeasurement(rulerSpanPixels(m_start.y(), m_current.y()), m_unit, m_ppiTenths);
        }
        return QStringLiteral("D=")
               + formatRulerMeasurement(rulerDistancePixels(m_start, m_current), m_unit, m_ppiTenths);
    }

    void finish()
    {
        const QString        text = measurementText();
        const FinishCallback callback = m_finish;
        hide();
        callback(text);
        deleteLater();
    }

    void cancel()
    {
        const CancelCallback callback = m_cancel;
        hide();
        callback();
        deleteLater();
    }

    CaptureFrame   m_frame;
    bool           m_isRectangle{false};
    RulerUnit      m_unit{RulerUnit::kPixels};
    int            m_ppiTenths{960};
    FinishCallback m_finish;
    CancelCallback m_cancel;
    QPoint         m_start;
    QPoint         m_current;
    bool           m_hasOrigin{false};
    bool           m_dragging{false};
    bool           m_isWaitingSecondClick{false};
};
} // namespace

ScreenRulerWindow::ScreenRulerWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowMinimizeButtonHint
                   | Qt::NoDropShadowWindowHint);
    setObjectName(QStringLiteral("screenRuler"));
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowOpacity(1.0);
    createControls();
}

ScreenRulerWindow::~ScreenRulerWindow() = default;

void ScreenRulerWindow::showAtCursor()
{
    QRect screen_rect = targetScreenRect();
    QSize target_size = m_orientation == RulerOrientation::kVertical
                            ? QSize(RULER_THICKNESS, qMin(m_verticalLength, screen_rect.height() - 20))
                            : QSize(qMin(m_horizontalLength, screen_rect.width() - 20), RULER_THICKNESS);
    target_size = target_size.expandedTo(QSize(RULER_THICKNESS, RULER_THICKNESS));
    resize(target_size);

    QPoint position = QCursor::pos() + QPoint(24, 24);
    if (position.x() + width() > screen_rect.right()) {
        position.setX(QCursor::pos().x() - width() - 24);
    }
    if (position.y() + height() > screen_rect.bottom()) {
        position.setY(QCursor::pos().y() - height() - 24);
    }
    move(clampedPosition(screen_rect, QRect(position, size())));
    show();
    raise();
    activateWindow();
    setFocus(Qt::OtherFocusReason);
    setStayOnTop(m_stayOnTop);
    update();
}

void ScreenRulerWindow::setOrientation(RulerOrientation orientation)
{
    if (m_orientation == orientation) {
        return;
    }
    m_orientation = orientation;
    QSize       next = orientation == RulerOrientation::kVertical
                           ? QSize(RULER_THICKNESS, qMax(MIN_VERTICAL_LENGTH, width()))
                           : QSize(qMax(MIN_HORIZONTAL_LENGTH, height()), RULER_THICKNESS);
    const QRect screen_rect = targetScreenRect();
    next.setWidth(qMin(next.width(), screen_rect.width()));
    next.setHeight(qMin(next.height(), screen_rect.height()));
    resize(next);
    layoutControls();
    clampToScreen();
    update();
}

RulerOrientation ScreenRulerWindow::orientation() const
{
    return m_orientation;
}

void ScreenRulerWindow::setUnit(RulerUnit unit)
{
    m_unit = unit;
    update();
}

RulerUnit ScreenRulerWindow::unit() const
{
    return m_unit;
}

QString ScreenRulerWindow::lastMeasurement() const
{
    return m_lastMeasurement;
}

void ScreenRulerWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(rect(), m_background);
    painter.setPen(QPen(m_foreground, 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    QFont font = painter.font();
    font.setPixelSize(9);
    painter.setFont(font);

    const bool vertical = m_orientation == RulerOrientation::kVertical;
    const int  length = axisLength();
    for (int coordinate = 0; coordinate < length; coordinate += 5) {
        int tick_length = 5;
        if (coordinate % 50 == 0) {
            tick_length = 22;
        } else if (coordinate % 10 == 0) {
            tick_length = 12;
        }
        if (vertical) {
            painter.drawLine(0, coordinate, tick_length, coordinate);
            painter.drawLine(width() - tick_length - 1, coordinate, width() - 1, coordinate);
        } else {
            painter.drawLine(coordinate, 0, coordinate, tick_length);
            painter.drawLine(coordinate, height() - tick_length - 1, coordinate, height() - 1);
        }

        if (coordinate > 0 && coordinate % 100 == 0) {
            const QString text = QString::number(coordinate);
            if (vertical) {
                painter.drawText(QRect(24, coordinate - 7, 52, 15), Qt::AlignLeft | Qt::AlignVCenter, text);
                painter.drawText(QRect(width() - 76, coordinate - 7, 52, 15), Qt::AlignRight | Qt::AlignVCenter, text);
            } else {
                painter.drawText(QRect(coordinate - 25, 21, 50, 14), Qt::AlignCenter, text);
                painter.drawText(QRect(coordinate - 25, height() - 34, 50, 14), Qt::AlignCenter, text);
            }
        }
    }

    painter.setPen(QPen(m_foreground, 1));
    painter.setBrush(m_foreground);
    if (vertical) {
        painter.drawPolygon(QPolygon{QPoint(width() / 2, 3), QPoint(width() / 2 - 5, 10), QPoint(width() / 2 + 5, 10)});
        painter.drawPolygon(QPolygon{QPoint(width() / 2, height() - 4),
                                     QPoint(width() / 2 - 5, height() - 11),
                                     QPoint(width() / 2 + 5, height() - 11)});
    } else {
        painter.drawPolygon(
            QPolygon{QPoint(3, height() / 2), QPoint(10, height() / 2 - 5), QPoint(10, height() / 2 + 5)});
        painter.drawPolygon(QPolygon{QPoint(width() - 4, height() / 2),
                                     QPoint(width() - 11, height() / 2 - 5),
                                     QPoint(width() - 11, height() / 2 + 5)});
    }

    font.setBold(true);
    font.setPixelSize(12);
    painter.setFont(font);
    const QString size_text = vertical ? QStringLiteral("H=%1 px").arg(height())
                                       : QStringLiteral("W=%1 px").arg(width());
    if (vertical) {
        painter.drawText(QRect(23, height() / 2 + 28, width() - 30, 20), Qt::AlignLeft | Qt::AlignVCenter, size_text);
    } else {
        painter.drawText(QRect(30, height() - 28, 150, 20), Qt::AlignLeft | Qt::AlignVCenter, size_text);
    }
    if (!m_lastMeasurement.isEmpty()) {
        if (vertical) {
            painter.drawText(QRect(23, height() / 2 + 52, width() - 30, 20),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             m_lastMeasurement);
        } else {
            painter.drawText(QRect(30, height() - 52, qMin(300, width() - 40), 20),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             m_lastMeasurement);
        }
    }

    if (m_hasTickMeasurement) {
        painter.setPen(QPen(m_accent, 2));
        if (vertical) {
            painter.drawLine(0, m_tickStart, width(), m_tickStart);
            painter.drawLine(0, m_tickEnd, width(), m_tickEnd);
            painter.drawText(QRect(23, qMin(m_tickStart, m_tickEnd) - 10, width() - 30, 20),
                             Qt::AlignCenter,
                             formatRulerMeasurement(rulerSpanPixels(m_tickStart, m_tickEnd), m_unit, m_ppiTenths));
        } else {
            painter.drawLine(m_tickStart, 0, m_tickStart, height());
            painter.drawLine(m_tickEnd, 0, m_tickEnd, height());
            painter.drawText(QRect(qMin(m_tickStart, m_tickEnd), height() / 2 - 10, 150, 20),
                             Qt::AlignCenter,
                             formatRulerMeasurement(rulerSpanPixels(m_tickStart, m_tickEnd), m_unit, m_ppiTenths));
        }
    }
}

void ScreenRulerWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_dragMode = hitTest(event->position().toPoint());
    m_dragStartGlobal = event->globalPosition().toPoint();
    m_dragStartGeometry = geometry();
    if (m_dragMode == DragMode::kMeasureNear || m_dragMode == DragMode::kMeasureFar) {
        m_tickStart = axisCoordinate(event->position().toPoint());
        m_tickEnd = m_tickStart;
        m_hasTickMeasurement = true;
    }
    update();
}

void ScreenRulerWindow::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint position = event->position().toPoint();
    if (m_dragMode == DragMode::kNone) {
        m_hoverMode = hitTest(position);
        updateCursor(position);
        return;
    }
    const QPoint delta = event->globalPosition().toPoint() - m_dragStartGlobal;
    if (m_dragMode == DragMode::kMove) {
        const QRect screen_rect = targetScreenRect();
        move(clampedPosition(screen_rect, m_dragStartGeometry.translated(delta)));
    } else if (m_dragMode == DragMode::kMeasureNear || m_dragMode == DragMode::kMeasureFar) {
        m_tickEnd = qBound(0, axisCoordinate(position), axisLength() - 1);
    } else {
        QRect next = m_dragStartGeometry;
        if (m_orientation == RulerOrientation::kVertical) {
            if (m_dragMode == DragMode::kResizeEnd) {
                next.setHeight(qMax(MIN_VERTICAL_LENGTH, m_dragStartGeometry.height() + delta.y()));
            } else {
                const int next_height = qMax(MIN_VERTICAL_LENGTH, m_dragStartGeometry.height() - delta.y());
                next.setTop(m_dragStartGeometry.bottom() - next_height + 1);
            }
        } else if (m_dragMode == DragMode::kResizeEnd) {
            next.setWidth(qMax(MIN_HORIZONTAL_LENGTH, m_dragStartGeometry.width() + delta.x()));
        } else {
            const int next_width = qMax(MIN_HORIZONTAL_LENGTH, m_dragStartGeometry.width() - delta.x());
            next.setLeft(m_dragStartGeometry.right() - next_width + 1);
        }
        setGeometry(next);
        clampToScreen();
    }
    update();
}

void ScreenRulerWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragMode = DragMode::kNone;
    }
}

void ScreenRulerWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    const DragMode mode = hitTest(event->position().toPoint());
    if (mode == DragMode::kMeasureNear || mode == DragMode::kMeasureFar) {
        const QPoint delta = m_orientation == RulerOrientation::kVertical ? QPoint(0, event->position().toPoint().y())
                                                                          : QPoint(event->position().toPoint().x(), 0);
        move(pos() + delta);
        m_hasTickMeasurement = false;
        clampToScreen();
        update();
        return;
    }
    if (mode == DragMode::kMove) {
        setOrientation(m_orientation == RulerOrientation::kVertical ? RulerOrientation::kHorizontal
                                                                    : RulerOrientation::kVertical);
    }
}

void ScreenRulerWindow::contextMenuEvent(QContextMenuEvent *event)
{
    showOptionsMenu(event->globalPos());
}

void ScreenRulerWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    if (event->key() == Qt::Key_H) {
        setOrientation(RulerOrientation::kHorizontal);
        return;
    }
    if (event->key() == Qt::Key_V) {
        setOrientation(RulerOrientation::kVertical);
        return;
    }
    if (event->key() == Qt::Key_P) {
        setUnit(RulerUnit::kPixels);
        return;
    }
    if (event->key() == Qt::Key_C) {
        setUnit(RulerUnit::kCentimeters);
        return;
    }
    if (event->key() == Qt::Key_I) {
        setUnit(RulerUnit::kInches);
        return;
    }
    if (event->key() == Qt::Key_Plus) {
        setOpacityPercent(m_opacityPercent + 5);
        return;
    }
    if (event->key() == Qt::Key_Minus) {
        setOpacityPercent(m_opacityPercent - 5);
        return;
    }
    QWidget::keyPressEvent(event);
}

void ScreenRulerWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutControls();
}

void ScreenRulerWindow::closeEvent(QCloseEvent *event)
{
    if (m_measurementOverlay) {
        m_measurementOverlay->close();
    }
    emit sigClosed();
    QWidget::closeEvent(event);
}

void ScreenRulerWindow::createControls()
{
    m_controls = new QWidget(this);
    m_controls->setObjectName(QStringLiteral("rulerControls"));
    QLayout *layout = nullptr;
    if (m_orientation == RulerOrientation::kVertical) {
        layout = new QVBoxLayout(m_controls);
    } else {
        layout = new QHBoxLayout(m_controls);
    }
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    const struct
    {
        GlyphIcon icon;
        QString   object_name;
        QString   tooltip;
    } controls[] = {
        {GlyphIcon::kRulerOrientation, QStringLiteral("rulerOrientation"), QStringLiteral("切换水平/垂直")},
        {GlyphIcon::kMeasureRectangle, QStringLiteral("rulerRectangle"), QStringLiteral("矩形区域测量")},
        {GlyphIcon::kMeasureDistance, QStringLiteral("rulerDistance"), QStringLiteral("点到点距离")},
        {GlyphIcon::kOptions, QStringLiteral("rulerOptions"), QStringLiteral("标尺选项")},
        {GlyphIcon::kMinimize, QStringLiteral("rulerMinimize"), QStringLiteral("最小化")},
        {GlyphIcon::kClose, QStringLiteral("rulerClose"), QStringLiteral("关闭标尺")},
    };
    for (int index = 0; index < static_cast<int>(std::size(controls)); ++index) {
        auto *button = new QToolButton(m_controls);
        button->setObjectName(controls[index].object_name);
        button->setIcon(createGlyphIcon(controls[index].icon, m_foreground));
        button->setIconSize(QSize(16, 16));
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setToolTip(controls[index].tooltip);
        button->setFixedSize(24, 24);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(button);
        m_controlButtons.append(button);
        connect(button, &QToolButton::clicked, this, [this, index]() {
            if (index == 0) {
                setOrientation(m_orientation == RulerOrientation::kVertical ? RulerOrientation::kHorizontal
                                                                            : RulerOrientation::kVertical);
            } else if (index == 1) {
                startFullMeasurement(FullMeasureMode::kRectangle);
            } else if (index == 2) {
                startFullMeasurement(FullMeasureMode::kDistance);
            } else if (index == 3) {
                showOptionsMenu(
                    m_controlButtons.at(index)->mapToGlobal(m_controlButtons.at(index)->rect().bottomLeft()));
            } else if (index == 4) {
                showMinimized();
            } else {
                close();
            }
        });
    }
}

void ScreenRulerWindow::layoutControls()
{
    if (m_controls == nullptr) {
        return;
    }
    auto *old_layout = m_controls->layout();
    if (old_layout != nullptr) {
        delete old_layout;
    }
    QBoxLayout *layout = nullptr;
    if (m_orientation == RulerOrientation::kVertical) {
        layout = new QVBoxLayout(m_controls);
    } else {
        layout = new QHBoxLayout(m_controls);
    }
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    for (QToolButton *button : m_controlButtons) {
        layout->addWidget(button);
    }
    const QSize controls_size = m_orientation == RulerOrientation::kVertical ? QSize(24, 144) : QSize(144, 24);
    m_controls->setFixedSize(controls_size);
    m_controls->move(m_orientation == RulerOrientation::kVertical ? QPoint((width() - 24) / 2, 44)
                                                                  : QPoint(44, (height() - 24) / 2));
    m_controls->raise();
}

void ScreenRulerWindow::showOptionsMenu(const QPoint &global_position)
{
    QMenu    menu;
    QAction *horizontal = menu.addAction(QStringLiteral("水平"));
    horizontal->setCheckable(true);
    horizontal->setChecked(m_orientation == RulerOrientation::kHorizontal);
    QAction *vertical = menu.addAction(QStringLiteral("垂直"));
    vertical->setCheckable(true);
    vertical->setChecked(m_orientation == RulerOrientation::kVertical);
    menu.addSeparator();
    QAction *pixels = menu.addAction(QStringLiteral("像素"));
    QAction *centimeters = menu.addAction(QStringLiteral("厘米"));
    QAction *inches = menu.addAction(QStringLiteral("英寸"));
    pixels->setCheckable(true);
    centimeters->setCheckable(true);
    inches->setCheckable(true);
    pixels->setChecked(m_unit == RulerUnit::kPixels);
    centimeters->setChecked(m_unit == RulerUnit::kCentimeters);
    inches->setChecked(m_unit == RulerUnit::kInches);
    QAction *calibrate = menu.addAction(QStringLiteral("校准标尺（每英寸像素数量）"));
    menu.addSeparator();
    QAction *stay_on_top = menu.addAction(QStringLiteral("总在最前面"));
    stay_on_top->setCheckable(true);
    stay_on_top->setChecked(m_stayOnTop);
    QAction *color = menu.addAction(QStringLiteral("颜色"));
    QMenu   *opacity = menu.addMenu(QStringLiteral("不透明度"));
    QAction *auto_copy = menu.addAction(QStringLiteral("自动发送到剪贴板"));
    auto_copy->setCheckable(true);
    auto_copy->setChecked(m_autoCopy);
    menu.addSeparator();
    QAction *reset = menu.addAction(QStringLiteral("重置位置"));
    QAction *minimize = menu.addAction(QStringLiteral("最小化"));
    QAction *close_action = menu.addAction(QStringLiteral("关闭"));

    for (int percent = 40; percent <= 100; percent += 5) {
        QAction *action = opacity->addAction(QStringLiteral("%1%").arg(percent));
        action->setCheckable(true);
        action->setChecked(percent == m_opacityPercent);
        connect(action, &QAction::triggered, this, [this, percent]() { setOpacityPercent(percent); });
    }

    QAction *selected = menu.exec(global_position);
    if (selected == horizontal) {
        setOrientation(RulerOrientation::kHorizontal);
    } else if (selected == vertical) {
        setOrientation(RulerOrientation::kVertical);
    } else if (selected == pixels) {
        setUnit(RulerUnit::kPixels);
    } else if (selected == centimeters) {
        setUnit(RulerUnit::kCentimeters);
    } else if (selected == inches) {
        setUnit(RulerUnit::kInches);
    } else if (selected == calibrate) {
        bool         ok = false;
        const double ppi = QInputDialog::getDouble(this,
                                                   QStringLiteral("校准标尺"),
                                                   QStringLiteral("每英寸像素数量："),
                                                   m_ppiTenths / 10.0,
                                                   50.0,
                                                   10000.0,
                                                   1,
                                                   &ok);
        if (ok) {
            m_ppiTenths = qRound(ppi * 10.0);
            update();
        }
    } else if (selected == stay_on_top) {
        setStayOnTop(!m_stayOnTop);
    } else if (selected == color) {
        const QColor selected_color = QColorDialog::getColor(m_background, this, QStringLiteral("标尺颜色"));
        if (selected_color.isValid()) {
            m_background = selected_color;
            update();
        }
    } else if (selected == auto_copy) {
        m_autoCopy = !m_autoCopy;
    } else if (selected == reset) {
        showAtCursor();
    } else if (selected == minimize) {
        showMinimized();
    } else if (selected == close_action) {
        close();
    }
}

void ScreenRulerWindow::startFullMeasurement(FullMeasureMode mode)
{
    if (m_measurementOverlay) {
        return;
    }
    hide();
    QTimer::singleShot(80, this, [this, mode]() {
        const CaptureFrame frame = captureVirtualDesktop(false);
        if (!frame.isValid()) {
            show();
            return;
        }
        auto *overlay = new RulerMeasurementOverlay(
            frame,
            mode == FullMeasureMode::kRectangle,
            m_unit,
            m_ppiTenths,
            [this](const QString &text) { finishFullMeasurement(text); },
            [this]() { cancelFullMeasurement(); },
            this);
        m_measurementOverlay = overlay;
        overlay->begin();
    });
}

void ScreenRulerWindow::finishFullMeasurement(const QString &text)
{
    m_measurementOverlay = nullptr;
    m_lastMeasurement = text;
    if (m_autoCopy) {
        QApplication::clipboard()->setText(text);
    }
    show();
    raise();
    activateWindow();
    update();
}

void ScreenRulerWindow::cancelFullMeasurement()
{
    m_measurementOverlay = nullptr;
    show();
    raise();
    activateWindow();
    update();
}

void ScreenRulerWindow::setOpacityPercent(int percent)
{
    m_opacityPercent = qBound(40, percent, 100);
    setWindowOpacity(m_opacityPercent / 100.0);
}

void ScreenRulerWindow::setStayOnTop(bool enabled)
{
    m_stayOnTop = enabled;
    const QPoint old_position = pos();
    setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    move(old_position);
    show();
    raise();
}

void ScreenRulerWindow::clampToScreen()
{
    const QRect screen_rect = targetScreenRect();
    move(clampedPosition(screen_rect, geometry()));
}

ScreenRulerWindow::DragMode ScreenRulerWindow::hitTest(const QPoint &position) const
{
    if (m_orientation == RulerOrientation::kVertical) {
        if (position.x() < TICK_ZONE) {
            return DragMode::kMeasureNear;
        }
        if (position.x() > width() - TICK_ZONE) {
            return DragMode::kMeasureFar;
        }
        if (position.y() < RESIZE_ZONE) {
            return DragMode::kResizeStart;
        }
        if (position.y() > height() - RESIZE_ZONE) {
            return DragMode::kResizeEnd;
        }
    } else {
        if (position.y() < TICK_ZONE) {
            return DragMode::kMeasureNear;
        }
        if (position.y() > height() - TICK_ZONE) {
            return DragMode::kMeasureFar;
        }
        if (position.x() < RESIZE_ZONE) {
            return DragMode::kResizeStart;
        }
        if (position.x() > width() - RESIZE_ZONE) {
            return DragMode::kResizeEnd;
        }
    }
    return DragMode::kMove;
}

int ScreenRulerWindow::axisLength() const
{
    return m_orientation == RulerOrientation::kVertical ? height() : width();
}

int ScreenRulerWindow::axisCoordinate(const QPoint &position) const
{
    return m_orientation == RulerOrientation::kVertical ? position.y() : position.x();
}

void ScreenRulerWindow::updateCursor(const QPoint &position)
{
    const DragMode mode = hitTest(position);
    if (mode == DragMode::kResizeStart || mode == DragMode::kResizeEnd) {
        setCursor(m_orientation == RulerOrientation::kVertical ? Qt::SizeVerCursor : Qt::SizeHorCursor);
    } else if (mode == DragMode::kMeasureNear || mode == DragMode::kMeasureFar) {
        setCursor(m_orientation == RulerOrientation::kVertical ? Qt::SizeHorCursor : Qt::SizeVerCursor);
    } else {
        setCursor(Qt::SizeAllCursor);
    }
}

QScreen *ScreenRulerWindow::rulerScreen() const
{
    if (QScreen *screen = QGuiApplication::screenAt(frameGeometry().center())) {
        return screen;
    }
    return QGuiApplication::primaryScreen();
}

QRect ScreenRulerWindow::targetScreenRect() const
{
    QScreen *screen = rulerScreen();
    return screen != nullptr ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
}
