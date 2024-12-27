#include "magnifier.h"
#include <QPainter>

Magnifier::Magnifier() {}

void Magnifier::paint(QPainter &painter, const QImage &background, const QPoint &pos)
{
    // 计算放大区域
    int    grabSize = MAGNIFIER_SIZE / ZOOM_FACTOR;
    QPoint grabTopLeft = pos - QPoint(grabSize / 2, grabSize / 2);

    // 创建放大后的图像
    QImage magnified = background.copy(grabTopLeft.x(), grabTopLeft.y(), grabSize, grabSize)
                           .scaled(MAGNIFIER_SIZE,
                                   MAGNIFIER_SIZE,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);

    // 计算放大镜位置
    QPoint magPos = pos + QPoint(20, 20);

    // 如果放大镜超出右边界，放到左边
    if (magPos.x() + MAGNIFIER_SIZE > background.width()) {
        magPos.setX(pos.x() - MAGNIFIER_SIZE - 20);
    }

    // 如果放大镜超出下边界，放到上边
    if (magPos.y() + MAGNIFIER_SIZE + 40 > background.height()) {
        magPos.setY(pos.y() - MAGNIFIER_SIZE - 60);
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制放大区域背景
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawRect(magPos.x(), magPos.y(), MAGNIFIER_SIZE, MAGNIFIER_SIZE);

    // 绘制放大后的图像
    painter.drawImage(magPos, magnified);

    // 绘制边框
    painter.setPen(QPen(QColor(0, 0, 0, 180), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(magPos.x(), magPos.y(), MAGNIFIER_SIZE - 1, MAGNIFIER_SIZE - 1);

    // 绘制十字线
    int centerX = magPos.x() + MAGNIFIER_SIZE / 2;
    int centerY = magPos.y() + MAGNIFIER_SIZE / 2;

    // 获取像素颜色
    QColor pixelColor = background.pixelColor(pos);

    // 绘制十字线（线宽与放大倍数相同）
    painter.setPen(QPen(QColor(178, 211, 250, 180), ZOOM_FACTOR));
    painter.drawLine(magPos.x() + 4, centerY, magPos.x() + MAGNIFIER_SIZE - 4, centerY);
    painter.drawLine(centerX, magPos.y() + 4, centerX, magPos.y() + MAGNIFIER_SIZE - 4);

    // 在十字线交叉点绘制当前像素
    QRect pixelRect(centerX - ZOOM_FACTOR / 2, centerY - ZOOM_FACTOR / 2, ZOOM_FACTOR, ZOOM_FACTOR);
    painter.setPen(Qt::NoPen);
    painter.setBrush(pixelColor);
    painter.drawRect(pixelRect);

    // 绘制信息区域背景（半透明）
    QRect infoRect(magPos.x(), magPos.y() + MAGNIFIER_SIZE, MAGNIFIER_SIZE, 40);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.drawRect(infoRect);

    // 设置字体
    QFont font = painter.font();
    font.setPixelSize(12);
    painter.setFont(font);

    // 获取字体度量以计算高度
    QFontMetrics fm(font);
    int          textHeight = fm.height();
    int          infoHeight = infoRect.height();
    int          lineSpacing = (infoHeight - textHeight * 2) / 3; // 三等分空白区域

    // 绘制坐标信息（居中对齐）
    painter.setPen(Qt::white);
    QString posText = QString("(%1, %2)").arg(pos.x()).arg(pos.y());
    QRect   posRect = infoRect.adjusted(5, lineSpacing, -5, -textHeight - lineSpacing);
    painter.drawText(posRect, Qt::AlignCenter, posText);

    // 绘制颜色预览方块（与文字等高）
    int   colorBlockY = infoRect.top() + lineSpacing * 2 + textHeight;
    QRect colorPreview(magPos.x() + 15, colorBlockY, textHeight, textHeight);
    painter.setPen(Qt::NoPen);
    painter.setBrush(pixelColor);
    painter.drawRect(colorPreview);

    // 绘制颜色值（居中对齐）
    QString colorText = QString(" %1, %2, %3")
                            .arg(pixelColor.red())
                            .arg(pixelColor.green())
                            .arg(pixelColor.blue());
    QRect colorTextRect(colorPreview.right() + 5,
                        colorBlockY,
                        infoRect.width() - colorPreview.width() - 15,
                        textHeight);
    painter.setPen(Qt::white);
    painter.drawText(colorTextRect, Qt::AlignLeft | Qt::AlignVCenter, colorText);

    painter.restore();
}
