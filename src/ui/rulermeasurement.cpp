#include "rulermeasurement.h"

#include <QtMath>

#include <cmath>

int rulerSpanPixels(int start, int end)
{
    return start == end ? 0 : qAbs(end - start) + 1;
}

int rulerDistancePixels(const QPoint &start, const QPoint &end)
{
    const double width = rulerSpanPixels(start.x(), end.x());
    const double height = rulerSpanPixels(start.y(), end.y());
    return static_cast<int>(std::nearbyint(std::hypot(width, height)));
}

QString formatRulerMeasurement(int pixels, RulerUnit unit, int ppi_tenths)
{
    switch (unit) {
    case RulerUnit::kPixels:
        return QStringLiteral("%1 px").arg(pixels);
    case RulerUnit::kCentimeters: {
        const qreal ppi = qMax(1, ppi_tenths) / 10.0;
        return QStringLiteral("%1 cm").arg(pixels * 2.54 / ppi, 0, 'f', 1);
    }
    case RulerUnit::kInches: {
        const qreal ppi = qMax(1, ppi_tenths) / 10.0;
        return QStringLiteral("%1 in").arg(pixels / ppi, 0, 'f', 2);
    }
    }
    return {};
}
