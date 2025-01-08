#ifndef FREEHAND_H
#define FREEHAND_H

#include "shape.h"
#include <QColor>
#include <QPoint>
#include <QVector>

class Freehand : public Shape
{
public:
    Freehand(const QPoint &startPoint, const QColor &color = Qt::black);
    ~Freehand() override = default;

    void   draw(QPainter &painter) const override;
    void   updateShape(const QPoint &pos) override;
    bool   isComplete() const override;
    Shape *clone() const override;

private:
    QVector<QPoint> m_points;
    QColor          m_color;
    bool            m_complete = false;

    static constexpr int PEN_WIDTH = 2;
};

#endif // FREEHAND_H
