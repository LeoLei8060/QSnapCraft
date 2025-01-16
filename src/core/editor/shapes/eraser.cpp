#include "eraser.h"

#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>

Eraser::Eraser(const QPoint &startPoint)
    : Shape(startPoint)
{
    m_points.append(startPoint);
}

void Eraser::draw(QPainter &painter) const
{
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.setPen(Qt::NoPen);

    // 使用 QPainterPath 定义橡皮擦路径
    QPainterPath path;
    if (m_points.size() > 1) {
        path.moveTo(m_points[0]);
        for (int i = 1; i < m_points.size(); ++i) {
            path.lineTo(m_points[i]);
        }
    }

    // 设置橡皮擦的宽度
    QPen eraserPen(Qt::transparent, 10); // 宽度为 10
    painter.strokePath(path, eraserPen);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
}

void Eraser::updateShape(const QPoint &pos)
{
    if (!m_complete) {
        m_points.append(pos);
    }
}

bool Eraser::isComplete() const
{
    return m_complete;
}

Shape *Eraser::clone() const
{
    auto *newShape = new Eraser(m_start);
    newShape->m_points = m_points;
    newShape->m_complete = m_complete;
    return newShape;
}
