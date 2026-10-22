#include "paintitem.h"

#include <QPainter>
#include <QPainterPathStroker>
#include <QtMath>

namespace {
constexpr int EFFECT_TILE_SIZE = 128;
constexpr int EFFECT_TILES_PER_STEP = 4;

QImage blurredRegion(const QImage &source, const QRect &region, int radius)
{
    const QRect clipped = region.intersected(source.rect());
    if (clipped.isEmpty() || radius <= 0) {
        return source;
    }

    const QImage input = source.copy(clipped).convertToFormat(QImage::Format_ARGB32);
    QImage       horizontal(input.size(), QImage::Format_ARGB32);
    const int    diameter = radius * 2 + 1;

    for (int y = 0; y < input.height(); ++y) {
        const QRgb *input_line = reinterpret_cast<const QRgb *>(input.constScanLine(y));
        QRgb       *output_line = reinterpret_cast<QRgb *>(horizontal.scanLine(y));
        int         red = 0;
        int         green = 0;
        int         blue = 0;
        int         alpha = 0;
        for (int offset = -radius; offset <= radius; ++offset) {
            const QRgb pixel = input_line[qBound(0, offset, input.width() - 1)];
            red += qRed(pixel);
            green += qGreen(pixel);
            blue += qBlue(pixel);
            alpha += qAlpha(pixel);
        }
        for (int x = 0; x < input.width(); ++x) {
            output_line[x] = qRgba(red / diameter, green / diameter, blue / diameter, alpha / diameter);
            const QRgb removed = input_line[qBound(0, x - radius, input.width() - 1)];
            const QRgb added = input_line[qBound(0, x + radius + 1, input.width() - 1)];
            red += qRed(added) - qRed(removed);
            green += qGreen(added) - qGreen(removed);
            blue += qBlue(added) - qBlue(removed);
            alpha += qAlpha(added) - qAlpha(removed);
        }
    }

    QImage blurred(input.size(), QImage::Format_ARGB32);
    for (int x = 0; x < input.width(); ++x) {
        int red = 0;
        int green = 0;
        int blue = 0;
        int alpha = 0;
        for (int offset = -radius; offset <= radius; ++offset) {
            const int  y = qBound(0, offset, input.height() - 1);
            const QRgb pixel = reinterpret_cast<const QRgb *>(horizontal.constScanLine(y))[x];
            red += qRed(pixel);
            green += qGreen(pixel);
            blue += qBlue(pixel);
            alpha += qAlpha(pixel);
        }
        for (int y = 0; y < input.height(); ++y) {
            reinterpret_cast<QRgb *>(
                blurred.scanLine(y))[x] = qRgba(red / diameter, green / diameter, blue / diameter, alpha / diameter);
            const int  removed_y = qBound(0, y - radius, input.height() - 1);
            const int  added_y = qBound(0, y + radius + 1, input.height() - 1);
            const QRgb removed = reinterpret_cast<const QRgb *>(horizontal.constScanLine(removed_y))[x];
            const QRgb added = reinterpret_cast<const QRgb *>(horizontal.constScanLine(added_y))[x];
            red += qRed(added) - qRed(removed);
            green += qGreen(added) - qGreen(removed);
            blue += qBlue(added) - qBlue(removed);
            alpha += qAlpha(added) - qAlpha(removed);
        }
    }

    return blurred;
}
} // namespace

PaintItem::PaintItem(PaintTool tool, const QPointF &startPoint, const QColor &color, qreal width)
    : m_tool(tool)
    , m_color(color)
    , m_penWidth(width)
{
    m_points.append(startPoint);
    rebuildPath();
}

QRectF PaintItem::boundingRect() const
{
    if (m_path.isEmpty()) {
        return QRectF(m_points.value(0), QSizeF(1, 1)).normalized();
    }
    const qreal margin = m_tool == PaintTool::kArrow ? qMax<qreal>(12.0, m_penWidth * 6.0)
                                                     : qMax<qreal>(2.0, m_penWidth);
    return m_path.boundingRect().adjusted(-margin, -margin, margin, margin);
}

QPainterPath PaintItem::shape() const
{
    return m_path;
}

void PaintItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor strokeColor = m_color;
    if (m_tool == PaintTool::kMarker) {
        strokeColor.setAlpha(m_markerOpacity);
    }
    QPen pen(strokeColor,
             m_tool == PaintTool::kMarker ? qMax<qreal>(12.0, m_penWidth) : m_penWidth,
             Qt::SolidLine,
             Qt::RoundCap,
             Qt::RoundJoin);
    painter->setPen(pen);
    painter->setBrush(m_filled ? QBrush(m_color) : Qt::NoBrush);

    if (m_tool == PaintTool::kNumber) {
        const QPointF center = m_points.value(0);
        const qreal   radius = qMax<qreal>(12.0, m_penWidth * 6.0);
        painter->setBrush(m_color);
        painter->setPen(QPen(Qt::white, 1));
        painter->drawEllipse(center, radius, radius);
        QFont font = painter->font();
        font.setBold(true);
        font.setPixelSize(qRound(radius));
        painter->setFont(font);
        painter->setPen(Qt::white);
        painter->drawText(QRectF(center.x() - radius, center.y() - radius, radius * 2, radius * 2),
                          Qt::AlignCenter,
                          QString::number(m_number));
    } else if ((m_tool == PaintTool::kEraser || m_tool == PaintTool::kMosaic) && !m_sourceImage.isNull()) {
        QPainterPath clipPath;
        if (m_tool == PaintTool::kMosaic) {
            clipPath = effectClipPath();
        } else if (m_filled) {
            clipPath.addRect(m_path.boundingRect());
        } else {
            clipPath = QPainterPathStroker(pen).createStroke(m_path);
        }
        painter->setClipPath(clipPath);
        if (m_tool == PaintTool::kEraser) {
            painter->drawImage(QPoint(0, 0), m_sourceImage);
        } else {
            painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
            for (auto it = m_effectTiles.constBegin(); it != m_effectTiles.constEnd(); ++it) {
                const QRect tile_rect = effectTileRect(it.key());
                if (tile_rect.isValid()) {
                    painter->drawImage(tile_rect.topLeft(), it.value());
                }
            }
        }
    } else if (m_tool == PaintTool::kArrow) {
        paintArrow(painter);
    } else {
        painter->drawPath(m_path);
    }
    painter->restore();
    if (isSelected()) {
        painter->save();
        QPen selectionPen(Qt::white, 1, Qt::DashLine);
        selectionPen.setCosmetic(true);
        painter->setPen(selectionPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));
        const QRectF bounds = boundingRect();
        painter->setPen(QPen(Qt::black, 1));
        painter->setBrush(Qt::white);
        const QPointF handles[] = {bounds.topLeft(), bounds.topRight(), bounds.bottomLeft(), bounds.bottomRight()};
        for (const QPointF &handle : handles) {
            painter->drawRect(QRectF(handle.x() - 3, handle.y() - 3, 6, 6));
        }
        painter->restore();
    }
}

void PaintItem::updatePoint(const QPointF &point)
{
    if (m_points.isEmpty()) {
        m_points.append(point);
    } else if (m_tool == PaintTool::kPencil || m_tool == PaintTool::kMarker
               || (m_tool == PaintTool::kMosaic && !m_filled) || m_tool == PaintTool::kEraser) {
        appendPoint(point);
        return;
    } else if (m_points.size() == 1) {
        m_points.append(point);
    } else {
        m_points.last() = point;
    }
    prepareGeometryChange();
    rebuildPath();
    refreshEffectTiles();
    processEffectTiles(EFFECT_TILES_PER_STEP);
    update();
}

void PaintItem::appendPoint(const QPointF &point)
{
    prepareGeometryChange();
    m_points.append(point);
    rebuildPath();
    refreshEffectTiles();
    processEffectTiles(EFFECT_TILES_PER_STEP);
    update();
}

PaintItem *PaintItem::clone() const
{
    auto *copy = new PaintItem(m_tool, m_points.value(0), m_color, m_penWidth);
    copy->m_points = m_points;
    copy->m_filled = m_filled;
    copy->m_arrowStyle = m_arrowStyle;
    copy->m_markerOpacity = m_markerOpacity;
    copy->m_effectMode = m_effectMode;
    copy->m_effectLevels = m_effectLevels;
    copy->m_number = m_number;
    copy->m_sourceImage = m_sourceImage;
    copy->m_effectTiles = m_effectTiles;
    copy->m_pendingEffectTiles = m_pendingEffectTiles;
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setScale(scale());
    copy->setZValue(zValue());
    copy->rebuildPath();
    return copy;
}

PaintTool PaintItem::tool() const
{
    return m_tool;
}

QColor PaintItem::color() const
{
    return m_color;
}

void PaintItem::setColor(const QColor &color)
{
    if (color.isValid()) {
        m_color = color;
        update();
    }
}

qreal PaintItem::penWidth() const
{
    return m_penWidth;
}

void PaintItem::setPenWidth(qreal width)
{
    prepareGeometryChange();
    m_penWidth = qMax<qreal>(0.5, width);
    refreshEffectTiles();
    processEffectTiles(EFFECT_TILES_PER_STEP);
    update();
}

void PaintItem::setFilled(bool filled)
{
    m_filled = filled;
    refreshEffectTiles();
    processEffectTiles(EFFECT_TILES_PER_STEP);
    update();
}

bool PaintItem::isFilled() const
{
    return m_filled;
}

void PaintItem::setArrowStyle(ArrowStyle style)
{
    if (m_arrowStyle != style) {
        m_arrowStyle = style;
        update();
    }
}

ArrowStyle PaintItem::arrowStyle() const
{
    return m_arrowStyle;
}

void PaintItem::setMarkerOpacity(int opacity)
{
    m_markerOpacity = qBound(0, opacity, 255);
    update();
}

int PaintItem::markerOpacity() const
{
    return m_markerOpacity;
}

void PaintItem::setEffectMode(PaintEffectMode mode)
{
    if (m_effectMode != mode) {
        m_effectMode = mode;
        invalidateEffectTiles();
        update();
    }
}

PaintEffectMode PaintItem::effectMode() const
{
    return m_effectMode;
}

void PaintItem::setEffectLevel(int level)
{
    setEffectLevel(m_effectMode, level);
}

void PaintItem::setEffectLevel(PaintEffectMode mode, int level)
{
    m_effectLevels[static_cast<std::size_t>(mode)] = qMax(1, level);
    invalidateEffectTiles();
    update();
}

int PaintItem::effectLevel() const
{
    return effectLevel(m_effectMode);
}

int PaintItem::effectLevel(PaintEffectMode mode) const
{
    return m_effectLevels[static_cast<std::size_t>(mode)];
}

void PaintItem::setSourceImage(const QImage &image)
{
    m_sourceImage = image;
    invalidateEffectTiles();
}

void PaintItem::setNumber(int number)
{
    m_number = qMax(1, number);
    update();
}

int PaintItem::number() const
{
    return m_number;
}

const QVector<QPointF> &PaintItem::points() const
{
    return m_points;
}

bool PaintItem::hasPendingEffectTiles() const
{
    return !m_pendingEffectTiles.isEmpty();
}

void PaintItem::processEffectTiles(int maximum)
{
    int processed = 0;
    while (!m_pendingEffectTiles.isEmpty() && processed < maximum) {
        auto         it = m_pendingEffectTiles.begin();
        const QPoint key = *it;
        m_pendingEffectTiles.erase(it);

        if (m_effectTiles.contains(key)) {
            continue;
        }
        const QRect tile_rect = effectTileRect(key);
        if (!tile_rect.isValid()) {
            continue;
        }
        const QImage tile = renderEffectTile(tile_rect);
        if (!tile.isNull()) {
            m_effectTiles.insert(key, tile);
            ++processed;
        }
    }
    if (processed > 0) {
        update();
    }
}

void PaintItem::flushEffectTiles()
{
    while (!m_pendingEffectTiles.isEmpty()) {
        const int remaining = m_pendingEffectTiles.size();
        processEffectTiles(remaining);
    }
}

void PaintItem::rebuildPath()
{
    m_path = QPainterPath();
    if (m_points.isEmpty()) {
        return;
    }

    const QPointF first = m_points.first();
    const QPointF last = m_points.last();
    switch (m_tool) {
    case PaintTool::kRectangle:
        m_path.addRect(QRectF(first, last).normalized());
        break;
    case PaintTool::kEllipse:
        m_path.addEllipse(QRectF(first, last).normalized());
        break;
    case PaintTool::kLine:
    case PaintTool::kArrow:
        m_path.moveTo(first);
        m_path.lineTo(last);
        break;
    case PaintTool::kPencil:
    case PaintTool::kMarker:
    case PaintTool::kMosaic:
    case PaintTool::kEraser:
        m_path.moveTo(first);
        for (int index = 1; index < m_points.size(); ++index) {
            m_path.lineTo(m_points.at(index));
        }
        break;
    case PaintTool::kNumber:
        m_path.addEllipse(first, 12, 12);
        break;
    case PaintTool::kText:
    case PaintTool::kSelect:
        break;
    }
}

void PaintItem::paintArrow(QPainter *painter)
{
    if (m_points.size() < 2) {
        return;
    }

    const QPointF start = m_points.first();
    const QPointF end = m_points.last();
    const QPointF direction = end - start;
    if (direction.isNull()) {
        return;
    }

    const qreal   length = std::hypot(direction.x(), direction.y());
    const QPointF unit(direction.x() / length, direction.y() / length);
    const QPointF normal(-unit.y(), unit.x());
    const qreal   headLength = qMin(qMax<qreal>(16.0, m_penWidth * 8.0), length * 0.75);
    const qreal   headHalfWidth = headLength * 0.5;
    const QPointF headBase = end - unit * headLength;
    const QPointF left = headBase + normal * headHalfWidth;
    const QPointF right = headBase - normal * headHalfWidth;

    if (m_arrowStyle == ArrowStyle::kSolid) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_color);
        painter->drawPolygon(QPolygonF{end, left, right});
        painter->setPen(QPen(m_color, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawLine(start, headBase);
        return;
    }

    const qreal   tailHalfWidth = qMax<qreal>(1.0, m_penWidth * 0.28);
    const qreal   shoulderHalfWidth = qMax<qreal>(tailHalfWidth + 1.0, headHalfWidth * 0.28);
    const QPointF tailLeft = start + normal * tailHalfWidth;
    const QPointF tailRight = start - normal * tailHalfWidth;
    const QPointF shoulderLeft = headBase + normal * shoulderHalfWidth;
    const QPointF shoulderRight = headBase - normal * shoulderHalfWidth;
    QPainterPath  arrow;
    arrow.moveTo(end);
    arrow.lineTo(left);
    arrow.lineTo(shoulderLeft);
    arrow.lineTo(tailLeft);
    arrow.lineTo(tailRight);
    arrow.lineTo(shoulderRight);
    arrow.lineTo(right);
    arrow.closeSubpath();

    if (m_arrowStyle == ArrowStyle::kSolidTail) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_color);
    } else {
        painter->setPen(QPen(m_color, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
    }
    painter->drawPath(arrow);
}

QPainterPath PaintItem::effectClipPath() const
{
    if (m_tool != PaintTool::kMosaic) {
        return {};
    }
    if (m_filled) {
        QPainterPath path;
        path.addRect(m_path.boundingRect());
        return path;
    }
    const QPen pen(Qt::black, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    return QPainterPathStroker(pen).createStroke(m_path);
}

void PaintItem::invalidateEffectTiles()
{
    m_effectTiles.clear();
    refreshEffectTiles();
}

void PaintItem::refreshEffectTiles()
{
    m_pendingEffectTiles.clear();
    if (m_tool != PaintTool::kMosaic || m_sourceImage.isNull()) {
        return;
    }

    const QPainterPath clip_path = effectClipPath();
    const QRectF clip_bounds = clip_path.boundingRect().intersected(QRectF(m_sourceImage.rect()));
    if (clip_bounds.isEmpty()) {
        return;
    }

    const int left = qMax(0, qFloor(clip_bounds.left() / EFFECT_TILE_SIZE) * EFFECT_TILE_SIZE);
    const int top = qMax(0, qFloor(clip_bounds.top() / EFFECT_TILE_SIZE) * EFFECT_TILE_SIZE);
    const int right = qMin(m_sourceImage.width() - 1, qCeil(clip_bounds.right()));
    const int bottom = qMin(m_sourceImage.height() - 1, qCeil(clip_bounds.bottom()));

    QSet<QPoint> desired_tiles;
    for (int y = top; y <= bottom; y += EFFECT_TILE_SIZE) {
        for (int x = left; x <= right; x += EFFECT_TILE_SIZE) {
            const QPoint key(x, y);
            const QRect tile_rect = effectTileRect(key);
            if (!tile_rect.isValid() || !clip_path.intersects(QRectF(tile_rect))) {
                continue;
            }
            desired_tiles.insert(key);
        }
    }

    for (auto it = m_effectTiles.begin(); it != m_effectTiles.end();) {
        if (desired_tiles.contains(it.key())) {
            ++it;
        } else {
            it = m_effectTiles.erase(it);
        }
    }
    for (const QPoint &key : desired_tiles) {
        if (!m_effectTiles.contains(key)) {
            m_pendingEffectTiles.insert(key);
        }
    }
}

QRect PaintItem::effectTileRect(const QPoint &key) const
{
    const QSize tile_size(qMin(EFFECT_TILE_SIZE, m_sourceImage.width() - key.x()),
                          qMin(EFFECT_TILE_SIZE, m_sourceImage.height() - key.y()));
    return QRect(key, tile_size).intersected(m_sourceImage.rect());
}

QImage PaintItem::renderEffectTile(const QRect &tile_rect) const
{
    const int effect_level = effectLevel();
    if (m_effectMode == PaintEffectMode::kBlur) {
        const int  radius = qBound(1, effect_level, 30);
        const QRect sample_rect =
            tile_rect.adjusted(-radius, -radius, radius, radius).intersected(m_sourceImage.rect());
        const QImage blurred = blurredRegion(m_sourceImage, sample_rect, radius);
        const QRect  local_rect(tile_rect.topLeft() - sample_rect.topLeft(), tile_rect.size());
        return blurred.copy(local_rect);
    }

    const int   block = qBound(2, effect_level, 64);
    const int   left = (tile_rect.left() / block) * block;
    const int   top = (tile_rect.top() / block) * block;
    const int   right = qMin(m_sourceImage.width(), ((tile_rect.right() + block) / block) * block);
    const int   bottom = qMin(m_sourceImage.height(), ((tile_rect.bottom() + block) / block) * block);
    const QRect sample_rect = QRect(QPoint(left, top), QSize(right - left, bottom - top))
                                  .intersected(m_sourceImage.rect());
    const QImage input = m_sourceImage.copy(sample_rect);
    const QSize  reduced(qMax(1, sample_rect.width() / block), qMax(1, sample_rect.height() / block));
    const QImage pixelated = input.scaled(reduced, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                 .scaled(sample_rect.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
    const QRect  local_rect(tile_rect.topLeft() - sample_rect.topLeft(), tile_rect.size());
    return pixelated.copy(local_rect);
}
