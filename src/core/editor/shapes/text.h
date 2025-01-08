#ifndef TEXT_H
#define TEXT_H

#include "shape.h"
#include <QColor>
#include <QFont>
#include <QPixmap>
#include <QRect>
#include <QString>

class Text : public Shape
{
public:
    Text(const QPoint &pos, const QColor &color = Qt::black);
    ~Text() override = default;

    void   draw(QPainter &painter) const override;
    void   updateShape(const QPoint &pos) override;
    bool   isComplete() const override;
    Shape *clone() const override;

    void           setText(const QString &text);
    void           appendText(const QString &text);
    void           removeLastChar();
    void           finishEdit();
    const QString &getText() const { return m_text; }
    bool           isEmpty() const { return m_text.isEmpty(); }
    void           startMove(const QPoint &pos);
    void           updateBackgroundColor(const QColor &color);
    void           setBackground(const QPixmap &bg) { m_background = bg; }
    void           setFont(const QFont &font) { this->m_font = font; }

    const QRect &getTextRect() const { return m_textRect; }
    const QFont &getFont() const { return m_font; }
    int          getPadding() const { return m_padding; }

private:
    QPoint    m_position;
    QString   m_text;
    bool      m_editing = true;
    QRect     m_textRect;
    QFont     m_font{"Microsoft YaHei", 12};
    const int m_padding = 5;     // 输入框内边距
    QPoint    m_moveOffset;      // 移动时的偏移量
    QColor    m_backgroundColor; // 背景颜色
    QPixmap   m_background;      // 背景图像

    void   updateTextRect();
    QColor getContrastColor(const QColor &color) const; // 获取与指定颜色的对比色
    void   drawDashedRect(QPainter &painter, const QRect &rect) const; // 绘制虚线边框
};

#endif // TEXT_H
