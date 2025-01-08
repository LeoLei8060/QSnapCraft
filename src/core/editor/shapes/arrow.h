#ifndef ARROW_H
#define ARROW_H

#include "shape.h"
#include <QColor>
#include <QPoint>
#include <QPolygon>

class Arrow : public Shape
{
public:
    Arrow(const QPoint &startPoint, const QColor &color = Qt::black);
    ~Arrow() override = default;

    void   draw(QPainter &painter) const override;
    void   updateShape(const QPoint &pos) override;
    bool   isComplete() const override;
    Shape *clone() const override;

private:
    QPoint m_startPoint;
    QPoint m_endPoint;
    QColor m_color;
    bool   m_complete = false;

    static constexpr double ARROW_HEAD_SIZE = 20.0;  // 箭头大小
    static constexpr double ARROW_HEAD_ANGLE = 30.0; // 箭头角度（度）

    QPolygon createArrowHead() const;
};

#endif // ARROW_H
