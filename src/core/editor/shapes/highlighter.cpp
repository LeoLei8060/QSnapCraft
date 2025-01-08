#include "highlighter.h"
#include <QtMath>

Highlighter::Highlighter(const QPoint &startPoint, const QColor &color)
    : Shape(startPoint, color)
{
    m_endPoint = startPoint;
}

void Highlighter::draw(QPainter &painter) const
{
    // 创建半透明颜色
    QColor markerColor = m_penColor;
    markerColor.setAlpha(MARKER_ALPHA);

    // 设置画笔
    QPen pen(markerColor, MARKER_WIDTH);
    pen.setCapStyle(Qt::RoundCap);   // 圆形线帽
    pen.setJoinStyle(Qt::RoundJoin); // 圆形连接
    painter.setPen(pen);

    // 启用抗锯齿
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制直线
    painter.drawLine(m_start, m_endPoint);
}

void Highlighter::updateShape(const QPoint &pos)
{
    if (!m_complete) {
        m_endPoint = pos;
    }
}

bool Highlighter::isComplete() const
{
    return m_complete;
}

Shape *Highlighter::clone() const
{
    auto *newShape = new Highlighter(m_start, m_penColor);
    newShape->m_endPoint = m_endPoint;
    newShape->m_complete = m_complete;
    return newShape;
}
