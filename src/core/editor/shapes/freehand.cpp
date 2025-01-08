#include "freehand.h"

Freehand::Freehand(const QPoint &startPoint, const QColor &color)
    : Shape(startPoint, color)
{
    m_points.append(startPoint);
}

void Freehand::draw(QPainter &painter) const
{
    painter.setPen(QPen(m_penColor, 2));
    if (m_points.size() > 1) {
        for (int i = 1; i < m_points.size(); ++i) {
            painter.drawLine(m_points[i - 1], m_points[i]);
        }
    }
}

void Freehand::updateShape(const QPoint &pos)
{
    if (!m_complete) {
        m_points.append(pos);
    }
}

bool Freehand::isComplete() const
{
    return m_complete;
}

Shape *Freehand::clone() const
{
    auto *newShape = new Freehand(m_start, m_penColor);
    newShape->m_points = m_points;
    newShape->m_complete = m_complete;
    return newShape;
}
