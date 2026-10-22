#include "painttextitem.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

PaintTextItem::PaintTextItem(const QString &text, QGraphicsItem *parent)
    : QGraphicsTextItem(text, parent)
{
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    applyCharacterFormat();
}

QRectF PaintTextItem::boundingRect() const
{
    return QGraphicsTextItem::boundingRect().adjusted(-4, -3, 4, 3);
}

QPainterPath PaintTextItem::shape() const
{
    QPainterPath result = QGraphicsTextItem::shape();
    result.addRect(QGraphicsTextItem::boundingRect().adjusted(-4, -3, 4, 3));
    return result.simplified();
}

void PaintTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (m_hasBackground) {
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_backgroundColor);
        painter->drawRoundedRect(QGraphicsTextItem::boundingRect().adjusted(-4, -3, 4, 3), 3, 3);
        painter->restore();
    }

    QGraphicsTextItem::paint(painter, option, widget);
    if (isSelected()) {
        painter->save();
        QPen selection_pen(Qt::white, 1, Qt::DashLine);
        selection_pen.setCosmetic(true);
        painter->setPen(selection_pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));
        const QRectF bounds = boundingRect();
        const QPointF handles[] = {
            bounds.topLeft(),
            QPointF(bounds.center().x(), bounds.top()),
            bounds.topRight(),
            QPointF(bounds.right(), bounds.center().y()),
            bounds.bottomRight(),
            QPointF(bounds.center().x(), bounds.bottom()),
            bounds.bottomLeft(),
            QPointF(bounds.left(), bounds.center().y()),
        };
        painter->setPen(QPen(Qt::black, 1));
        painter->setBrush(Qt::white);
        for (const QPointF &handle : handles) {
            painter->drawRect(QRectF(handle.x() - 3, handle.y() - 3, 6, 6));
        }
        painter->restore();
    }
}

PaintTextItem *PaintTextItem::clone() const
{
    auto *copy = new PaintTextItem(toPlainText());
    copy->setFont(font());
    copy->setTextColor(m_textColor);
    copy->setOutlineColor(m_outlineColor);
    copy->setOutlined(m_outlined);
    copy->setBackgroundColor(m_backgroundColor);
    copy->setBackgroundEnabled(m_hasBackground);
    copy->setPos(pos());
    copy->setRotation(rotation());
    copy->setTransform(transform());
    copy->setTransformOriginPoint(transformOriginPoint());
    copy->setZValue(zValue());
    return copy;
}

QColor PaintTextItem::textColor() const
{
    return m_textColor;
}

void PaintTextItem::setTextColor(const QColor &color)
{
    if (!color.isValid()) {
        return;
    }
    m_textColor = color;
    applyCharacterFormat();
}

bool PaintTextItem::isOutlined() const
{
    return m_outlined;
}

void PaintTextItem::setOutlined(bool outlined)
{
    m_outlined = outlined;
    applyCharacterFormat();
}

QColor PaintTextItem::outlineColor() const
{
    return m_outlineColor;
}

void PaintTextItem::setOutlineColor(const QColor &color)
{
    if (color.isValid()) {
        m_outlineColor = color;
        applyCharacterFormat();
    }
}

bool PaintTextItem::hasBackground() const
{
    return m_hasBackground;
}

void PaintTextItem::setBackgroundEnabled(bool enabled)
{
    if (m_hasBackground == enabled) {
        return;
    }
    prepareGeometryChange();
    m_hasBackground = enabled;
    update();
}

QColor PaintTextItem::backgroundColor() const
{
    return m_backgroundColor;
}

void PaintTextItem::setBackgroundColor(const QColor &color)
{
    if (color.isValid()) {
        m_backgroundColor = color;
        update();
    }
}

void PaintTextItem::keyPressEvent(QKeyEvent *event)
{
    QGraphicsTextItem::keyPressEvent(event);
    applyCharacterFormat();
}

void PaintTextItem::focusOutEvent(QFocusEvent *event)
{
    QGraphicsTextItem::focusOutEvent(event);
    applyCharacterFormat();
}

void PaintTextItem::applyCharacterFormat()
{
    QTextCursor cursor(document());
    cursor.select(QTextCursor::Document);
    QTextCharFormat format;
    format.setForeground(m_textColor);
    format.setTextOutline(m_outlined ? QPen(m_outlineColor, 1.0) : QPen(Qt::NoPen));
    cursor.mergeCharFormat(format);
    update();
}
