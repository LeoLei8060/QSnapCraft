#include "mosaic.h"
#include <QPainter>
#include <QPainterPath>

Mosaic::Mosaic(const QPoint &startPoint, MosaicType type, const QColor &color)
    : Shape(startPoint, color)
    , m_mosaicType(type)
{
    m_lastPoint = startPoint;
}

void Mosaic::draw(QPainter &painter) const
{
    if (m_mosaicAreas.isEmpty())
        return;

    painter.save();

    for (int i = 0; i < m_mosaicAreas.size(); ++i) {
        // 根据马赛克类型设置裁剪区域
        QPainterPath clipPath;
        if (m_mosaicType == MosaicType::Square) {
            clipPath.addRect(m_mosaicAreas[i]);
        } else { // Circle
            clipPath.addEllipse(m_mosaicAreas[i]);
        }
        painter.setClipPath(clipPath);

        // 绘制马赛克图像
        painter.drawImage(m_mosaicAreas[i], m_mosaicImages[i]);
    }

    painter.restore();
}

void Mosaic::updateShape(const QPoint &pos)
{
    if (!m_complete) {
        updateMosaicArea(pos);
        m_lastPoint = pos;
    }
}

void Mosaic::updateMosaicArea(const QPoint &pos)
{
    if (m_background.isNull())
        return;

    // 创建新的马赛克区域
    QRect newArea(pos.x() - BRUSH_SIZE / 2, pos.y() - BRUSH_SIZE / 2, BRUSH_SIZE, BRUSH_SIZE);

    // 获取背景图像对应区域
    QImage sourceImage = m_background.copy(newArea).toImage();

    // 创建马赛克图像
    QImage mosaicImage(newArea.size(), QImage::Format_ARGB32);
    mosaicImage.fill(Qt::transparent);

    // 按块处理图像
    for (int y = 0; y < sourceImage.height(); y += BLOCK_SIZE) {
        for (int x = 0; x < sourceImage.width(); x += BLOCK_SIZE) {
            // 计算当前块的区域
            QRect blockRect(x,
                            y,
                            qMin(BLOCK_SIZE, sourceImage.width() - x),
                            qMin(BLOCK_SIZE, sourceImage.height() - y));

            // 获取该区域的平均颜色
            QColor avgColor = getAverageColor(sourceImage, blockRect);

            // 填充马赛克块
            QPainter blockPainter(&mosaicImage);
            blockPainter.fillRect(blockRect, avgColor);
        }
    }

    // 添加到列表中
    m_mosaicAreas.append(newArea);
    m_mosaicImages.append(mosaicImage);
}

QColor Mosaic::getAverageColor(const QImage &image, const QRect &rect) const
{
    int r = 0, g = 0, b = 0, a = 0;
    int count = 0;

    // 计算区域内所有像素的颜色平均值
    for (int y = rect.top(); y < rect.bottom(); ++y) {
        for (int x = rect.left(); x < rect.right(); ++x) {
            QColor pixelColor = image.pixelColor(x, y);
            r += pixelColor.red();
            g += pixelColor.green();
            b += pixelColor.blue();
            a += pixelColor.alpha();
            ++count;
        }
    }

    if (count > 0) {
        return QColor(r / count, g / count, b / count, a / count);
    }
    return Qt::transparent;
}

bool Mosaic::isComplete() const
{
    return m_complete;
}

Shape *Mosaic::clone() const
{
    auto *newShape = new Mosaic(m_start, m_mosaicType, m_penColor);
    newShape->m_mosaicAreas = m_mosaicAreas;
    newShape->m_mosaicImages = m_mosaicImages;
    newShape->m_lastPoint = m_lastPoint;
    newShape->m_complete = m_complete;
    newShape->m_background = m_background;
    return newShape;
}
