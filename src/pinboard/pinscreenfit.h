#pragma once

#include <QPoint>
#include <QRect>
#include <QVector>

QPoint clampPinToScreens(const QRect &frameRect, const QVector<QRect> &availableScreens);
