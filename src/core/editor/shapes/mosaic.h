#ifndef MOSAIC_H
#define MOSAIC_H

#include "shape.h"
#include <QImage>
#include <QRect>
#include <QVector>

class Mosaic : public Shape
{
public:
    enum class MosaicType {
        Square = 0, // 方形马赛克
        Circle = 1  // 圆形马赛克
    };

    Mosaic(const QPoint &startPoint,
           MosaicType    type = MosaicType::Square,
           const QColor &color = Qt::black);
    void   draw(QPainter &painter) const override;
    void   updateShape(const QPoint &pos) override;
    bool   isComplete() const override;
    Shape *clone() const override;

    void setBackground(const QPixmap &bg) { m_background = bg; }
    void updateMosaicArea(const QPoint &pos);

private:
    QVector<QRect>  m_mosaicAreas;  // 存储所有马赛克区域
    QVector<QImage> m_mosaicImages; // 存储所有马赛克图像
    QPoint          m_lastPoint;    // 上一个点的位置
    bool            m_complete = false;
    MosaicType      m_mosaicType;
    QPixmap         m_background; // 背景图像

    static constexpr int BLOCK_SIZE = 10; // 马赛克方块大小
    static constexpr int BRUSH_SIZE = 20; // 马赛克笔刷大小
    QColor               getAverageColor(const QImage &image, const QRect &rect) const;
};

#endif // MOSAIC_H
