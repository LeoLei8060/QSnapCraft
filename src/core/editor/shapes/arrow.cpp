#include "arrow.h"
#include <QtMath>

Arrow::Arrow(const QPoint &startPoint, const QColor &color)
    : Shape(startPoint, color)
{
    m_endPoint = startPoint;
}

void Arrow::draw(QPainter &painter) const
{
    painter.setPen(QPen(m_penColor, 2));
    painter.setBrush(m_penColor);

    // 绘制主线
    painter.drawLine(m_start, m_endPoint);

    // 绘制箭头
    if (m_start != m_endPoint) {
        painter.drawPolygon(createArrowHead());
    }
}

void Arrow::updateShape(const QPoint &pos)
{
    if (!m_complete) {
        m_endPoint = pos;
    }
}

bool Arrow::isComplete() const
{
    return m_complete;
}

Shape *Arrow::clone() const
{
    auto *newShape = new Arrow(m_start, m_penColor);
    newShape->m_endPoint = m_endPoint;
    newShape->m_complete = m_complete;
    return newShape;
}

QPolygon Arrow::createArrowHead() const
{
    QPolygon arrowHead;
    QPointF  direction(m_endPoint - m_start);
    qreal    angle = qAtan2(-direction.y(), direction.x());

    // 计算箭头两个点的位置
    qreal arrowAngle1 = angle + qDegreesToRadians(ARROW_HEAD_ANGLE);
    qreal arrowAngle2 = angle - qDegreesToRadians(ARROW_HEAD_ANGLE);

    QPointF arrowPoint1(m_endPoint.x() - ARROW_HEAD_SIZE * qCos(arrowAngle1),
                        m_endPoint.y() + ARROW_HEAD_SIZE * qSin(arrowAngle1));
    QPointF arrowPoint2(m_endPoint.x() - ARROW_HEAD_SIZE * qCos(arrowAngle2),
                        m_endPoint.y() + ARROW_HEAD_SIZE * qSin(arrowAngle2));

    arrowHead << m_endPoint << arrowPoint1.toPoint() << arrowPoint2.toPoint();
    return arrowHead;
}
