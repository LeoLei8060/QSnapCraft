#pragma once

#include <QColor>
#include <QGraphicsTextItem>

class PaintTextItem : public QGraphicsTextItem
{
public:
    explicit PaintTextItem(const QString &text = QString(), QGraphicsItem *parent = nullptr);

    QRectF         boundingRect() const override;
    QPainterPath   shape() const override;
    void           paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    PaintTextItem *clone() const;

    QColor textColor() const;
    void   setTextColor(const QColor &color);
    bool   isOutlined() const;
    void   setOutlined(bool outlined);
    QColor outlineColor() const;
    void   setOutlineColor(const QColor &color);
    bool   hasBackground() const;
    void   setBackgroundEnabled(bool enabled);
    QColor backgroundColor() const;
    void   setBackgroundColor(const QColor &color);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void applyCharacterFormat();

    QColor m_textColor{255, 255, 255};
    QColor m_outlineColor{0, 0, 0};
    QColor m_backgroundColor{0, 0, 0, 180};
    bool   m_outlined{false};
    bool   m_hasBackground{false};
};
