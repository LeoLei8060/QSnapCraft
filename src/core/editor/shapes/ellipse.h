#ifndef ELLIPSE_H
#define ELLIPSE_H

#include "shape.h"
#include <QColor>
#include <QPoint>

class Ellipse : public Shape
{
public:
    Ellipse(const QPoint &startPoint, const QColor &color = Qt::black);
    ~Ellipse() override = default;

    void   draw(QPainter &painter) const override;
    void   updateShape(const QPoint &pos) override;
    bool   isComplete() const override;
    Shape *clone() const override;

    const QPoint &startPoint() const { return m_startPoint; }
    const QPoint &endPoint() const { return m_endPoint; }

private:
    QPoint m_startPoint;
    QPoint m_endPoint;
    bool   m_complete = false;

    QRect boundingRect() const;
};

#endif // ELLIPSE_H
