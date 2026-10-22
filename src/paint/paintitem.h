#pragma once

#include <QColor>
#include <QGraphicsItem>
#include <QHash>
#include <QImage>
#include <QPainterPath>
#include <QPointF>
#include <QRect>
#include <QSet>
#include <QVector>

#include "paint/paintscene.h"

#include <array>

class PaintItem : public QGraphicsItem
{
public:
    PaintItem(PaintTool tool, const QPointF &startPoint, const QColor &color, qreal width);

    QRectF       boundingRect() const override;
    QPainterPath shape() const override;
    void         paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void       updatePoint(const QPointF &point);
    void       appendPoint(const QPointF &point);
    PaintItem *clone() const;

    PaintTool               tool() const;
    QColor                  color() const;
    void                    setColor(const QColor &color);
    qreal                   penWidth() const;
    void                    setPenWidth(qreal width);
    void                    setFilled(bool filled);
    bool                    isFilled() const;
    void                    setArrowStyle(ArrowStyle style);
    ArrowStyle              arrowStyle() const;
    void                    setMarkerOpacity(int opacity);
    int                     markerOpacity() const;
    void                    setEffectMode(PaintEffectMode mode);
    PaintEffectMode         effectMode() const;
    void                    setEffectLevel(int level);
    void                    setEffectLevel(PaintEffectMode mode, int level);
    int                     effectLevel() const;
    int                     effectLevel(PaintEffectMode mode) const;
    void                    setSourceImage(const QImage &image);
    void                    setNumber(int number);
    int                     number() const;
    const QVector<QPointF> &points() const;
    bool                    hasPendingEffectTiles() const;
    void                    processEffectTiles(int maximum);
    void                    flushEffectTiles();

private:
    void         rebuildPath();
    void         paintArrow(QPainter *painter);
    void         invalidateEffectTiles();
    void         refreshEffectTiles();
    QPainterPath effectClipPath() const;
    QRect        effectTileRect(const QPoint &key) const;
    QImage       renderEffectTile(const QRect &tile_rect) const;

    PaintTool             m_tool;
    QVector<QPointF>      m_points;
    QPainterPath          m_path;
    QColor                m_color;
    qreal                 m_penWidth{2.0};
    bool                  m_filled{false};
    ArrowStyle            m_arrowStyle{ArrowStyle::kSolid};
    int                   m_markerOpacity{110};
    PaintEffectMode       m_effectMode{PaintEffectMode::kMosaic};
    std::array<int, 2>    m_effectLevels{8, 8};
    int                   m_number{1};
    QImage                m_sourceImage;
    QHash<QPoint, QImage> m_effectTiles;
    QSet<QPoint>          m_pendingEffectTiles;
};
