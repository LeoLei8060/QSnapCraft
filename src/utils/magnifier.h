#pragma once

#include <QImage>
#include <QPoint>

// 放大镜组件类，提供放大镜绘制功能
class Magnifier
{
public:
    Magnifier();

    // 绘制放大镜效果
    void paint(QPainter &painter, const QImage &background, const QPoint &pos);

private:
    static constexpr int MAGNIFIER_SIZE = 120; // 放大区域大小
    static constexpr int ZOOM_FACTOR = 6;      // 放大倍数
    static constexpr int CROSS_SIZE = 10;      // 十字线尺寸
};
