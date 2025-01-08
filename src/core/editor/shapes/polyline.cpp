#include "polyline.h"
#include <QDebug>

Polyline::Polyline(const QPoint &startPoint, const QColor &color)
    : Shape(startPoint, color)
{
    m_points.append(startPoint);
}

void Polyline::draw(QPainter &painter) const
{
    painter.setPen(QPen(m_penColor, 2));

    if (m_points.size() > 1) {
        for (int i = 1; i < m_points.size(); ++i) {
            painter.drawLine(m_points[i - 1], m_points[i]);
        }
    }

    if (!m_complete && m_hasPreview && !m_points.isEmpty()) {
        painter.drawLine(m_points.last(), m_previewPoint);
    }
}

void Polyline::updateShape(const QPoint &pos)
{
    if (!m_complete) {
        setPreviewPoint(pos);
    }
}

void Polyline::addPoint(const QPoint &point)
{
    if (!m_complete) {
        m_points.append(point);
    }
}

void Polyline::setPreviewPoint(const QPoint &point)
{
    if (!m_complete) {
        m_previewPoint = point;
        m_hasPreview = true;
    }
}

bool Polyline::isComplete() const
{
    return m_complete;
}

Shape *Polyline::clone() const
{
    auto *newShape = new Polyline(m_start, m_penColor);
    newShape->m_points = m_points;
    newShape->m_previewPoint = m_previewPoint;
    newShape->m_hasPreview = m_hasPreview;
    newShape->m_complete = m_complete;
    return newShape;
}

void Polyline::finishLine()
{
    if (!m_complete && m_points.size() >= 2) {
        m_complete = true;
    } else if (!m_complete && m_points.size() <= 1)
        m_isInvalid = true;
}
