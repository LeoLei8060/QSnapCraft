#include "pinscreenfit.h"

#include <QRegion>

#include <limits>

namespace {
qint64 squaredDistance(const QPoint &left, const QPoint &right)
{
    const qint64 x = left.x() - right.x();
    const qint64 y = left.y() - right.y();
    return x * x + y * y;
}

int clampedCoordinate(int value, int start, int end)
{
    return qBound(start, value, qMax(start, end));
}
} // namespace

QPoint clampPinToScreens(const QRect &frameRect, const QVector<QRect> &availableScreens)
{
    if (!frameRect.isValid() || availableScreens.isEmpty()) {
        return frameRect.topLeft();
    }

    QRegion screenArea;
    for (const QRect &screen : availableScreens) {
        if (screen.isValid()) {
            screenArea += screen;
        }
    }
    if (screenArea.contains(frameRect)) {
        return frameRect.topLeft();
    }

    QRect target;
    int   largestIntersection = 0;
    for (const QRect &screen : availableScreens) {
        const QRect intersection = frameRect.intersected(screen);
        const int   area = qMax(0, intersection.width()) * qMax(0, intersection.height());
        if (area > largestIntersection) {
            largestIntersection = area;
            target = screen;
        }
    }

    if (!target.isValid()) {
        qint64 bestDistance = std::numeric_limits<qint64>::max();
        for (const QRect &screen : availableScreens) {
            const qint64 distance = squaredDistance(frameRect.center(), screen.center());
            if (distance < bestDistance) {
                bestDistance = distance;
                target = screen;
            }
        }
    }
    if (!target.isValid()) {
        return frameRect.topLeft();
    }

    return QPoint(clampedCoordinate(frameRect.left(), target.left(), target.right() - frameRect.width() + 1),
                  clampedCoordinate(frameRect.top(), target.top(), target.bottom() - frameRect.height() + 1));
}
