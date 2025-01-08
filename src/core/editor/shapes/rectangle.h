#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "shape.h"
#include <QColor>
#include <QPoint>

class Rectangle : public Shape
{
public:
    Rectangle(const QPoint &startPoint, const QColor &color = Qt::black);
    ~Rectangle() override = default;

    void   draw(QPainter &painter) const override;
    void   updateShape(const QPoint &pos) override;
    bool   isComplete() const override;
    Shape *clone() const override;

private:
    QPoint m_startPoint;
    QPoint m_endPoint;
    QColor m_color;
    bool   m_complete = false;

    QRect boundingRect() const;
};

#endif // RECTANGLE_H
