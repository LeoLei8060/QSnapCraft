#include "text.h"
#include <QFontMetrics>

Text::Text(const QPoint &startPoint, const QColor &color)
    : Shape(startPoint, color)
    , m_position(startPoint)
    , m_backgroundColor(color)
    , m_font("Arial", 12)
{
    updateTextRect();
}

void Text::draw(QPainter &painter) const
{
    // 如果正在编辑，绘制虚线边框
    if (m_editing) {
        drawDashedRect(painter, m_textRect);
    }

    // 绘制文本
    if (!m_text.isEmpty()) {
        painter.setFont(m_font);
        painter.setPen(m_penColor);
        painter.drawText(m_textRect.adjusted(m_padding, m_padding, -m_padding, -m_padding),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         m_text);
    }
}

void Text::drawDashedRect(QPainter &painter, const QRect &rect) const
{
    if (m_background.isNull())
        return;

    QImage bgImage = m_background.toImage();

    // 绘制四条边
    for (int x = rect.left(); x <= rect.right(); ++x) {
        // 顶边
        if ((x - rect.left()) % 4 < 2) { // 虚线模式：2像素显示，2像素隐藏
            QColor bgColor = bgImage.pixelColor(x - rect.left(), 0);
            painter.setPen(getContrastColor(bgColor));
            painter.drawPoint(x, rect.top());
        }
        // 底边
        if ((x - rect.left()) % 4 < 2) {
            QColor bgColor = bgImage.pixelColor(x - rect.left(), rect.height() - 1);
            painter.setPen(getContrastColor(bgColor));
            painter.drawPoint(x, rect.bottom());
        }
    }

    for (int y = rect.top(); y <= rect.bottom(); ++y) {
        // 左边
        if ((y - rect.top()) % 4 < 2) {
            QColor bgColor = bgImage.pixelColor(0, y - rect.top());
            painter.setPen(getContrastColor(bgColor));
            painter.drawPoint(rect.left(), y);
        }
        // 右边
        if ((y - rect.top()) % 4 < 2) {
            QColor bgColor = bgImage.pixelColor(rect.width() - 1, y - rect.top());
            painter.setPen(getContrastColor(bgColor));
            painter.drawPoint(rect.right(), y);
        }
    }
}

void Text::updateShape(const QPoint &pos)
{
    // 更新位置时考虑偏移量
    m_position = pos - m_moveOffset;
    updateTextRect();
}

void Text::startMove(const QPoint &pos)
{
    // 计算鼠标点击位置相对于输入框左上角的偏移
    m_moveOffset = pos - m_position;
}

bool Text::isComplete() const
{
    return !m_editing;
}

Shape *Text::clone() const
{
    auto *cloned = new Text(m_position, m_backgroundColor);
    cloned->m_text = m_text;
    cloned->m_editing = m_editing;
    cloned->m_textRect = m_textRect;
    cloned->m_font = m_font;
    cloned->m_background = m_background;
    return cloned;
}

void Text::setText(const QString &newText)
{
    m_text = newText;
    updateTextRect();
}

void Text::appendText(const QString &appendText)
{
    m_text += appendText;
    updateTextRect();
}

void Text::removeLastChar()
{
    if (!m_text.isEmpty()) {
        m_text.chop(1);
        updateTextRect();
    }
}

void Text::finishEdit()
{
    m_editing = false;
}

void Text::updateBackgroundColor(const QColor &color)
{
    m_backgroundColor = color;
}

QColor Text::getContrastColor(const QColor &color) const
{
    // 计算亮度
    int brightness = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
    return brightness > 128 ? Qt::black : Qt::white;
}

void Text::updateTextRect()
{
    QFontMetrics metrics(m_font);
    int width = std::max(100, metrics.horizontalAdvance(m_text) + 2 * m_padding); // 最小宽度100
    int height = std::max(30, metrics.height() + 2 * m_padding);                  // 最小高度30
    m_textRect = QRect(m_position, QSize(width, height));
}
