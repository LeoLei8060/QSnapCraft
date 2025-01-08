#ifndef SHAPE_H
#define SHAPE_H

#include <QColor>
#include <QPainter>
#include <QPoint>

class Shape
{
public:
    Shape(const QPoint &startPoint, const QColor &color = Qt::black)
        : m_start(startPoint)
        , m_penColor(color)
    {}
    virtual ~Shape() = default;

    virtual void   draw(QPainter &painter) const = 0;
    virtual void   updateShape(const QPoint &pos) = 0;
    virtual bool   isComplete() const { return true; }
    virtual Shape *clone() const = 0;

    void   setPenColor(const QColor &color) { m_penColor = color; }
    QColor getPenColor() const { return m_penColor; }

protected:
    QPoint m_start;
    QColor m_penColor;
};

#endif // SHAPE_H
