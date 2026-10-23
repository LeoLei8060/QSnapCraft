#include "capturetypes.h"

#include <QPainter>

#include <algorithm>

bool renderCaptureSelection(const CaptureFrame &frame, const QRect &logicalSelection, QImage *image, QString *error)
{
    if (image == nullptr || !frame.isValid() || !logicalSelection.isValid()) {
        if (error != nullptr) {
            *error = QStringLiteral("截图数据或选区无效。");
        }
        return false;
    }

    const QRect selection = logicalSelection.intersected(frame.logicalBounds);
    if (!selection.isValid()) {
        if (error != nullptr) {
            *error = QStringLiteral("选区不在虚拟桌面范围内。");
        }
        return false;
    }

    const qreal outputScale = captureSelectionScale(frame, selection);

    const QSize outputSize(qMax(1, qRound(selection.width() * outputScale)),
                           qMax(1, qRound(selection.height() * outputScale)));
    QImage      output(outputSize, QImage::Format_ARGB32_Premultiplied);
    output.fill(Qt::transparent);

    QPainter painter(&output);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    for (const DisplaySurface &surface : frame.displays) {
        const QRect overlap = surface.logicalGeometry.intersected(selection);
        if (!overlap.isValid() || surface.nativeImage.isNull()) {
            continue;
        }

        const qreal surfaceScaleX = surface.logicalGeometry.width() > 0
                                        ? qreal(surface.nativeImage.width()) / surface.logicalGeometry.width()
                                        : 1.0;
        const qreal surfaceScaleY = surface.logicalGeometry.height() > 0
                                        ? qreal(surface.nativeImage.height()) / surface.logicalGeometry.height()
                                        : 1.0;

        const QRect source(qRound((overlap.left() - surface.logicalGeometry.left()) * surfaceScaleX),
                           qRound((overlap.top() - surface.logicalGeometry.top()) * surfaceScaleY),
                           qRound(overlap.width() * surfaceScaleX),
                           qRound(overlap.height() * surfaceScaleY));
        const QRect target(qRound((overlap.left() - selection.left()) * outputScale),
                           qRound((overlap.top() - selection.top()) * outputScale),
                           qRound(overlap.width() * outputScale),
                           qRound(overlap.height() * outputScale));

        painter.drawImage(target, surface.nativeImage, source);
    }

    if (frame.cursorIncluded && !frame.cursorImage.isNull() && selection.contains(frame.cursorGlobalPosition)) {
        const qreal   cursor_scale = frame.cursorScale > 0.0 ? frame.cursorScale : 1.0;
        const qreal   draw_scale = outputScale / cursor_scale;
        const QPointF cursor_point((frame.cursorGlobalPosition.x() - selection.left()) * outputScale,
                                   (frame.cursorGlobalPosition.y() - selection.top()) * outputScale);
        const QSizeF  cursor_size(frame.cursorImage.width() * draw_scale, frame.cursorImage.height() * draw_scale);
        const QPointF hotspot(frame.cursorHotspot.x() * draw_scale, frame.cursorHotspot.y() * draw_scale);
        painter.drawImage(QRectF(cursor_point - hotspot, cursor_size), frame.cursorImage);
    }
    painter.end();

    *image = std::move(output);
    return true;
}

qreal captureSelectionScale(const CaptureFrame &frame, const QRect &logical_selection)
{
    qreal scale = 1.0;
    for (const DisplaySurface &surface : frame.displays) {
        if (surface.logicalGeometry.intersects(logical_selection)) {
            scale = qMax(scale, surface.devicePixelRatio);
        }
    }
    return scale;
}
