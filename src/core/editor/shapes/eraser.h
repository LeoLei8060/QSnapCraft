#ifndef ERASER_H
#define ERASER_H

#include "shape.h"
#include <QColor>
#include <QPoint>
#include <QVector>

class Eraser : public Shape {
public:
    Eraser(const QPoint &startPoint);
    ~Eraser() override = default;
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

#endif // ERASER_H