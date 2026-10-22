#include "capturebargeometry.h"

#include <QtMath>

namespace {
QRect clampToScreen(const QRect &screenRect, const QSize &barSize, const QPoint &desiredTopLeft)
{
    const int    maxX = qMax(screenRect.left(), screenRect.right() - barSize.width() + 1);
    const int    maxY = qMax(screenRect.top(), screenRect.bottom() - barSize.height() + 1);
    const QPoint topLeft(qBound(screenRect.left(), desiredTopLeft.x(), maxX),
                         qBound(screenRect.top(), desiredTopLeft.y(), maxY));
    return QRect(topLeft, barSize);
}

} // namespace

DockPlacement calculateDockPlacement(const QRect  &screenRect,
                                     const QSize  &barSize,
                                     const QPoint &desiredTopLeft,
                                     int           snapDistance,
                                     int           hotspotThickness,
                                     int           hotspotLength)
{
    DockPlacement placement;
    placement.barRect = clampToScreen(screenRect, barSize, desiredTopLeft);

    const int distanceLeft = qAbs(placement.barRect.left() - screenRect.left());
    const int distanceRight = qAbs(screenRect.right() - placement.barRect.right());
    const int distanceTop = qAbs(placement.barRect.top() - screenRect.top());
    const int distanceBottom = qAbs(screenRect.bottom() - placement.barRect.bottom());

    int        nearestDistance = qMax(0, snapDistance) + 1;
    const auto consider = [&](DockEdge edge, int distance) {
        if (distance <= snapDistance && distance < nearestDistance) {
            nearestDistance = distance;
            placement.edge = edge;
        }
    };

    consider(DockEdge::Left, distanceLeft);
    consider(DockEdge::Right, distanceRight);
    consider(DockEdge::Top, distanceTop);
    consider(DockEdge::Bottom, distanceBottom);

    switch (placement.edge) {
    case DockEdge::Left:
        placement.barRect.moveLeft(screenRect.left());
        break;
    case DockEdge::Top:
        placement.barRect.moveTop(screenRect.top());
        break;
    case DockEdge::Right:
        placement.barRect.moveRight(screenRect.right());
        break;
    case DockEdge::Bottom:
        placement.barRect.moveBottom(screenRect.bottom());
        break;
    case DockEdge::None:
        break;
    }

    placement.hotspotRect = calculateDockHotspot(screenRect,
                                                 placement.barRect,
                                                 placement.edge,
                                                 hotspotThickness,
                                                 hotspotLength);
    return placement;
}

QRect placeAlongDock(const QRect &screenRect, const QSize &barSize, DockEdge edge, qreal positionRatio)
{
    const qreal ratio = qBound(0.0, positionRatio, 1.0);
    QRect       barRect(screenRect.topLeft(), barSize);

    switch (edge) {
    case DockEdge::Left:
        barRect.moveTop(screenRect.top() + qRound((screenRect.height() - barSize.height()) * ratio));
        barRect.moveLeft(screenRect.left());
        break;
    case DockEdge::Right:
        barRect.moveTop(screenRect.top() + qRound((screenRect.height() - barSize.height()) * ratio));
        barRect.moveRight(screenRect.right());
        break;
    case DockEdge::Top:
        barRect.moveLeft(screenRect.left() + qRound((screenRect.width() - barSize.width()) * ratio));
        barRect.moveTop(screenRect.top());
        break;
    case DockEdge::Bottom:
        barRect.moveLeft(screenRect.left() + qRound((screenRect.width() - barSize.width()) * ratio));
        barRect.moveBottom(screenRect.bottom());
        break;
    case DockEdge::None:
        barRect.moveCenter(screenRect.center());
        break;
    }

    return barRect;
}

QRect calculateDockHotspot(
    const QRect &screenRect, const QRect &barRect, DockEdge edge, int hotspotThickness, int hotspotLength)
{
    const int thickness = qMax(1, hotspotThickness);
    const int length = qMax(1, hotspotLength);

    switch (edge) {
    case DockEdge::Left: {
        const int height = qMin(length, screenRect.height());
        const int top = qBound(screenRect.top(), barRect.top(), screenRect.bottom() - height + 1);
        return QRect(screenRect.left(), top, qMin(thickness, screenRect.width()), height);
    }
    case DockEdge::Top: {
        const int width = qMin(length, screenRect.width());
        const int left = qBound(screenRect.left(), barRect.left(), screenRect.right() - width + 1);
        return QRect(left, screenRect.top(), width, qMin(thickness, screenRect.height()));
    }
    case DockEdge::Right: {
        const int height = qMin(length, screenRect.height());
        const int top = qBound(screenRect.top(), barRect.top(), screenRect.bottom() - height + 1);
        const int width = qMin(thickness, screenRect.width());
        return QRect(screenRect.right() - width + 1, top, width, height);
    }
    case DockEdge::Bottom: {
        const int width = qMin(length, screenRect.width());
        const int left = qBound(screenRect.left(), barRect.left(), screenRect.right() - width + 1);
        const int height = qMin(thickness, screenRect.height());
        return QRect(left, screenRect.bottom() - height + 1, width, height);
    }
    case DockEdge::None:
        break;
    }
    return {};
}

QString dockEdgeName(DockEdge edge)
{
    switch (edge) {
    case DockEdge::Left:
        return QStringLiteral("Left");
    case DockEdge::Top:
        return QStringLiteral("Top");
    case DockEdge::Right:
        return QStringLiteral("Right");
    case DockEdge::Bottom:
        return QStringLiteral("Bottom");
    case DockEdge::None:
        return QStringLiteral("None");
    }
    return QStringLiteral("None");
}

DockEdge dockEdgeFromName(const QString &name)
{
    if (name.compare(QStringLiteral("Left"), Qt::CaseInsensitive) == 0) {
        return DockEdge::Left;
    }
    if (name.compare(QStringLiteral("Top"), Qt::CaseInsensitive) == 0) {
        return DockEdge::Top;
    }
    if (name.compare(QStringLiteral("Right"), Qt::CaseInsensitive) == 0) {
        return DockEdge::Right;
    }
    if (name.compare(QStringLiteral("Bottom"), Qt::CaseInsensitive) == 0) {
        return DockEdge::Bottom;
    }
    return DockEdge::None;
}
