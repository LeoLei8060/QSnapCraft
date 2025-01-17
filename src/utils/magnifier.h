#pragma once

#include <QPixmap>
#include <QPoint>
#include <QColor>

// 放大镜组件类，提供放大镜绘制功能
class Magnifier
{
public:
    Magnifier();

    // 绘制放大镜效果
    void paint(QPainter &painter, const QPixmap &background, const QImage &img, const QPoint &pos);

    // 切换颜色显示格式
    void toggleColorFormat() { m_showHexColor = !m_showHexColor; }
    
    // 获取当前颜色显示格式
    bool isHexFormat() const { return m_showHexColor; }

    // 获取当前颜色的文本表示
    QString getCurrentColorText() const;

    // 设置当前颜色
    void setCurrentColor(const QColor &color) { m_currentColor = color; }

private:
    static constexpr int MAGNIFIER_SIZE = 120;    // 放大区域大小
    static constexpr int ZOOM_FACTOR = 6;         // 放大倍数
    static constexpr int CROSS_SIZE = 10;         // 十字线尺寸
    static constexpr int INFO_HEIGHT = 40;        // 颜色信息区域高度
    static constexpr int HINT_HEIGHT = 40;        // 提示信息区域高度
    static constexpr int FONT_SIZE = 12;          // 字体大小
    static constexpr int LINE_SPACING = 4;        // 行间距

    bool m_showHexColor{false};    // 是否显示十六进制颜色值
    QColor m_currentColor;         // 当前颜色

    // 辅助绘制函数
    void drawMagnifierArea(QPainter &painter, const QPoint &magPos, const QPixmap &magnified);
    void drawColorInfo(QPainter &painter, const QPoint &magPos, const QColor &pixelColor);
    void drawHintText(QPainter &painter, const QPoint &magPos);

    // 生成颜色文本
    QString formatColorText(const QColor &color) const;
};
