#pragma once

#include <QLineF>
#include <QRect>
#include <QVector>

struct PinSnapResult
{
    QRect           contentRect;
    QVector<QLineF> guides;
};

class PinSnapResolver
{
public:
    static PinSnapResult resolve(const QRect          &moving,
                                 const QRect          &screen,
                                 const QVector<QRect> &targets,
                                 int                   thresholdPx);
};
