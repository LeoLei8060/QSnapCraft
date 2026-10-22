#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>

enum class DockEdge { None, Left, Top, Right, Bottom };

struct DockPlacement
{
    DockEdge edge{DockEdge::None};
    QRect    barRect;
    QRect    hotspotRect;
};

DockPlacement calculateDockPlacement(const QRect  &screenRect,
                                     const QSize  &barSize,
                                     const QPoint &desiredTopLeft,
                                     int           snapDistance,
                                     int           hotspotThickness,
                                     int           hotspotLength = 50);

QRect placeAlongDock(const QRect &screenRect, const QSize &barSize, DockEdge edge, qreal positionRatio);

QRect calculateDockHotspot(
    const QRect &screenRect, const QRect &barRect, DockEdge edge, int hotspotThickness, int hotspotLength = 50);

QString  dockEdgeName(DockEdge edge);
DockEdge dockEdgeFromName(const QString &name);
