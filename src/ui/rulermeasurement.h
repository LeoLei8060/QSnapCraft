#pragma once

#include <QPoint>
#include <QString>

enum class RulerOrientation { kVertical, kHorizontal };
enum class RulerUnit { kPixels, kCentimeters, kInches };

int     rulerSpanPixels(int start, int end);
int     rulerDistancePixels(const QPoint &start, const QPoint &end);
QString formatRulerMeasurement(int pixels, RulerUnit unit, int ppi_tenths = 960);
