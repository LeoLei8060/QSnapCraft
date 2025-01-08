#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "shape.h"

class Highlighter : public Shape
{
public:
    Highlighter(const QPoint &startPoint, const QColor &color = Qt::black);
    void   draw(QPainter &painter) const override;
    void   updateShape(const QPoint &pos) override;
    bool   isComplete() const override;
    Shape *clone() const override;

private:
    QPoint m_endPoint;
    bool   m_complete = false;

    static constexpr int MARKER_WIDTH = 20; // 记号笔宽度
    static constexpr int MARKER_ALPHA = 50; // 透明度 (0-255)
};

#endif // HIGHLIGHTER_H
