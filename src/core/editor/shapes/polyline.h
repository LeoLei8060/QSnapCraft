#ifndef POLYLINE_H
#define POLYLINE_H

#include "shape.h"
#include <QColor>
#include <QPoint>
#include <QVector>

class Polyline : public Shape
{
public:
    Polyline(const QPoint &startPoint, const QColor &color = Qt::black);
    //    ~Polyline() override = default;

    void   draw(QPainter &painter) const override;
    void   updateShape(const QPoint &pos) override;
    bool   isComplete() const override;
    Shape *clone() const override;
    void   finishLine();

    void addPoint(const QPoint &point);
    void setPreviewPoint(const QPoint &point);

    bool isInvalid() const { return m_isInvalid; }

private:
    QVector<QPoint> m_points;
    QPoint          m_previewPoint;
    bool            m_hasPreview = false;
    bool            m_complete = false;
    bool            m_isInvalid = false;
    QColor          m_color;
};

#endif // POLYLINE_H
