#include "pinsnapoverlay.h"

#include <QPainter>

PinSnapOverlay::PinSnapOverlay(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput
                   | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
}

void PinSnapOverlay::setDesktopGeometry(const QRect &geometry)
{
    if (geometry.isValid() && geometry != this->geometry()) {
        setGeometry(geometry);
    }
}

void PinSnapOverlay::setGuides(const QVector<QLineF> &guides)
{
    m_guides = guides;
    if (m_guides.isEmpty()) {
        hide();
        update();
        return;
    }
    show();
    raise();
    update();
}

void PinSnapOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (m_guides.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen pen(QColor(10, 132, 255, 230));
    pen.setWidth(1);
    painter.setPen(pen);
    const QPoint offset = -pos();
    for (const QLineF &guide : m_guides) {
        painter.drawLine(guide.translated(offset));
    }
}
