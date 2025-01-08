#include "ellipse.h"

Ellipse::Ellipse(const QPoint &startPoint, const QColor &color)
    : Shape(startPoint, color)
{
    m_endPoint = startPoint;
}

void Ellipse::draw(QPainter &painter) const
{
    painter.setPen(QPen(m_penColor, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(boundingRect());
}

void Ellipse::updateShape(const QPoint &pos)
{
    if (!m_complete) {
        m_endPoint = pos;
    }
}

bool Ellipse::isComplete() const
{
    return m_complete;
}

Shape *Ellipse::clone() const
{
    auto *newShape = new Ellipse(m_start, m_penColor);
    newShape->m_endPoint = m_endPoint;
    newShape->m_complete = m_complete;
    return newShape;
}

QRect Ellipse::boundingRect() const
{
    return QRect(m_start, m_endPoint).normalized();
}
