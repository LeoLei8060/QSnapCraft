#include "rectangle.h"

Rectangle::Rectangle(const QPoint &startPoint, const QColor &color)
    : Shape(startPoint, color)
    , m_startPoint(startPoint)
    , m_endPoint(startPoint)
{}

void Rectangle::draw(QPainter &painter) const
{
    painter.setPen(QPen(m_penColor, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(boundingRect());
}

void Rectangle::updateShape(const QPoint &pos)
{
    if (!m_complete) {
        m_endPoint = pos;
    }
}

bool Rectangle::isComplete() const
{
    return m_complete;
}

Shape *Rectangle::clone() const
{
    auto *newShape = new Rectangle(m_start, m_penColor);
    newShape->m_endPoint = m_endPoint;
    newShape->m_complete = m_complete;
    return newShape;
}

QRect Rectangle::boundingRect() const
{
    return QRect(m_startPoint, m_endPoint).normalized();
}
