#include "glyphicon.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>
#include <QtMath>

#include <numbers>

namespace {
constexpr int DESIGN_SIZE = 32;

void drawGlyph(QPainter &painter, GlyphIcon glyph, const QColor &color)
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.8);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    switch (glyph) {
    case GlyphIcon::kCaptureRegion: {
        QPen dashed = pen;
        dashed.setStyle(Qt::DashLine);
        painter.setPen(dashed);
        painter.drawRect(QRectF(5, 7, 22, 18));
        painter.setPen(pen);
        const QPointF corners[] = {QPointF(5, 7), QPointF(27, 7), QPointF(5, 25), QPointF(27, 25)};
        for (const QPointF &corner : corners) {
            painter.drawLine(corner + QPointF(-2, 0), corner + QPointF(2, 0));
            painter.drawLine(corner + QPointF(0, -2), corner + QPointF(0, 2));
        }
        break;
    }
    case GlyphIcon::kActiveWindow:
        painter.drawRect(QRectF(4, 5, 24, 22));
        painter.drawLine(QPointF(4, 11), QPointF(28, 11));
        painter.drawRect(QRectF(9, 15, 14, 8));
        break;
    case GlyphIcon::kScrollCapture:
        painter.drawRect(QRectF(7, 4, 18, 24));
        painter.drawLine(QPointF(11, 10), QPointF(21, 10));
        painter.drawLine(QPointF(16, 7), QPointF(16, 22));
        painter.drawLine(QPointF(16, 7), QPointF(12, 11));
        painter.drawLine(QPointF(16, 7), QPointF(20, 11));
        painter.drawLine(QPointF(16, 22), QPointF(12, 18));
        painter.drawLine(QPointF(16, 22), QPointF(20, 18));
        break;
    case GlyphIcon::kOcr: {
        painter.drawLine(QPointF(4, 11), QPointF(4, 5));
        painter.drawLine(QPointF(4, 5), QPointF(10, 5));
        painter.drawLine(QPointF(22, 5), QPointF(28, 5));
        painter.drawLine(QPointF(28, 5), QPointF(28, 11));
        painter.drawLine(QPointF(4, 21), QPointF(4, 27));
        painter.drawLine(QPointF(4, 27), QPointF(10, 27));
        painter.drawLine(QPointF(22, 27), QPointF(28, 27));
        painter.drawLine(QPointF(28, 27), QPointF(28, 21));
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(13);
        painter.setFont(font);
        painter.drawText(QRectF(7, 8, 18, 16), Qt::AlignCenter, QStringLiteral("A"));
        break;
    }
    case GlyphIcon::kColorPicker: {
        painter.drawLine(QPointF(8, 24), QPointF(22, 10));
        QPolygonF dropper;
        dropper << QPointF(20, 6) << QPointF(26, 6) << QPointF(26, 12) << QPointF(22, 14) << QPointF(14, 22)
                << QPointF(9, 25) << QPointF(7, 23);
        painter.drawPolygon(dropper);
        painter.drawLine(QPointF(17, 9), QPointF(23, 15));
        break;
    }
    case GlyphIcon::kScreenRuler: {
        QPolygonF ruler;
        ruler << QPointF(6, 23) << QPointF(22, 5) << QPointF(27, 9) << QPointF(11, 27);
        painter.drawPolygon(ruler);
        painter.drawLine(QPointF(10, 20), QPointF(14, 24));
        painter.drawLine(QPointF(15, 15), QPointF(18, 18));
        painter.drawLine(QPointF(20, 10), QPointF(23, 13));
        break;
    }
    case GlyphIcon::kPreferences:
        painter.drawEllipse(QPointF(16, 16), 6, 6);
        painter.drawEllipse(QPointF(16, 16), 2, 2);
        for (int index = 0; index < 8; ++index) {
            const qreal   angle = index * std::numbers::pi / 4.0;
            const QPointF inner(16 + qCos(angle) * 8, 16 + qSin(angle) * 8);
            const QPointF outer(16 + qCos(angle) * 12, 16 + qSin(angle) * 12);
            painter.drawLine(inner, outer);
        }
        break;
    case GlyphIcon::kSelect: {
        QPolygonF cursor;
        cursor << QPointF(7, 4) << QPointF(24, 16) << QPointF(16, 18) << QPointF(20, 27) << QPointF(16, 29)
               << QPointF(12, 20) << QPointF(7, 24);
        painter.drawPolygon(cursor);
        break;
    }
    case GlyphIcon::kRectangle:
        painter.drawRect(QRectF(5, 7, 22, 18));
        break;
    case GlyphIcon::kEllipse:
        painter.drawEllipse(QRectF(5, 7, 22, 18));
        break;
    case GlyphIcon::kLine: {
        QPainterPath path;
        path.moveTo(5, 22);
        path.lineTo(11, 9);
        path.lineTo(20, 23);
        path.lineTo(27, 10);
        painter.drawPath(path);
        break;
    }
    case GlyphIcon::kArrow:
        painter.drawLine(QPointF(5, 25), QPointF(25, 7));
        painter.drawLine(QPointF(25, 7), QPointF(16, 8));
        painter.drawLine(QPointF(25, 7), QPointF(24, 16));
        break;
    case GlyphIcon::kPencil: {
        QPolygonF pencil;
        pencil << QPointF(7, 25) << QPointF(22, 7) << QPointF(27, 11) << QPointF(12, 29);
        painter.drawPolygon(pencil);
        painter.drawLine(QPointF(20, 9), QPointF(25, 13));
        break;
    }
    case GlyphIcon::kMarker:
        painter.setPen(QPen(color, 7, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(7, 22), QPointF(25, 22));
        break;
    case GlyphIcon::kMosaic:
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                if ((row + column) % 2 == 0) {
                    painter.fillRect(QRectF(7 + column * 6, 7 + row * 6, 5, 5), color);
                }
            }
        }
        break;
    case GlyphIcon::kText: {
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(21);
        painter.setFont(font);
        painter.drawText(QRectF(5, 4, 22, 24), Qt::AlignCenter, QStringLiteral("T"));
        break;
    }
    case GlyphIcon::kEraser: {
        QPolygonF eraser;
        eraser << QPointF(7, 24) << QPointF(20, 6) << QPointF(27, 12) << QPointF(15, 28);
        painter.drawPolygon(eraser);
        painter.drawLine(QPointF(11, 19), QPointF(22, 9));
        break;
    }
    case GlyphIcon::kNumber:
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(16, 16), 11, 11);
        painter.setPen(QPen(QColor(43, 43, 43), 1));
        {
            QFont font = painter.font();
            font.setBold(true);
            font.setPixelSize(15);
            painter.setFont(font);
            painter.drawText(QRectF(5, 5, 22, 22), Qt::AlignCenter, QStringLiteral("1"));
        }
        break;
    case GlyphIcon::kFill:
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(6, 6, 20, 20), 3, 3);
        break;
    case GlyphIcon::kUndo:
    case GlyphIcon::kRedo: {
        const bool   is_undo = glyph == GlyphIcon::kUndo;
        const QRectF arc = is_undo ? QRectF(6, 8, 20, 16) : QRectF(6, 8, 20, 16);
        painter.drawArc(arc, is_undo ? 35 * 16 : 145 * 16, 145 * 16);
        const QPointF tip = is_undo ? QPointF(6, 16) : QPointF(26, 16);
        const qreal   direction = is_undo ? 1 : -1;
        painter.drawLine(tip, tip + QPointF(direction * 5, -5));
        painter.drawLine(tip, tip + QPointF(direction * 5, 5));
        break;
    }
    case GlyphIcon::kCopy:
        painter.drawRect(QRectF(6, 6, 16, 18));
        painter.drawRect(QRectF(11, 10, 16, 18));
        break;
    case GlyphIcon::kSave:
        painter.drawLine(QPointF(16, 5), QPointF(16, 21));
        painter.drawLine(QPointF(16, 21), QPointF(10, 15));
        painter.drawLine(QPointF(16, 21), QPointF(22, 15));
        painter.drawLine(QPointF(5, 28), QPointF(27, 28));
        break;
    case GlyphIcon::kPin:
        painter.drawEllipse(QPointF(16, 16), 10, 10);
        painter.setBrush(color);
        painter.drawEllipse(QPointF(16, 16), 3, 3);
        break;
    case GlyphIcon::kClear:
        painter.drawRect(QRectF(9, 10, 15, 18));
        painter.drawLine(QPointF(7, 8), QPointF(26, 8));
        painter.drawLine(QPointF(13, 5), QPointF(21, 5));
        painter.drawLine(QPointF(14, 14), QPointF(14, 24));
        painter.drawLine(QPointF(19, 14), QPointF(19, 24));
        break;
    case GlyphIcon::kClose:
        painter.drawLine(QPointF(7, 7), QPointF(25, 25));
        painter.drawLine(QPointF(25, 7), QPointF(7, 25));
        break;
    case GlyphIcon::kBold: {
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(20);
        painter.setFont(font);
        painter.drawText(QRectF(6, 3, 20, 24), Qt::AlignCenter, QStringLiteral("B"));
        break;
    }
    case GlyphIcon::kItalic: {
        QFont font = painter.font();
        font.setItalic(true);
        font.setPixelSize(20);
        painter.setFont(font);
        painter.drawText(QRectF(6, 3, 20, 24), Qt::AlignCenter, QStringLiteral("I"));
        break;
    }
    case GlyphIcon::kTextOutline: {
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(17);
        painter.setFont(font);
        QPainterPath path;
        path.addText(7, 25, font, QStringLiteral("A"));
        painter.setPen(QPen(color, 1.2));
        painter.drawPath(path);
        painter.setPen(QPen(color, 1.8));
        painter.drawRect(QRectF(5, 5, 22, 22));
        break;
    }
    case GlyphIcon::kTextBackground:
        painter.setBrush(QColor(color.red(), color.green(), color.blue(), 85));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(5, 5, 22, 22), 3, 3);
        painter.setPen(pen);
        {
            QFont font = painter.font();
            font.setBold(true);
            font.setPixelSize(17);
            painter.setFont(font);
            painter.drawText(QRectF(6, 6, 20, 20), Qt::AlignCenter, QStringLiteral("A"));
        }
        break;
    case GlyphIcon::kRulerOrientation:
        painter.drawLine(QPointF(16, 10), QPointF(16, 22));
        painter.drawLine(QPointF(16, 10), QPointF(12, 14));
        painter.drawLine(QPointF(16, 10), QPointF(20, 14));
        painter.drawLine(QPointF(16, 22), QPointF(12, 18));
        painter.drawLine(QPointF(16, 22), QPointF(20, 18));
        break;
    case GlyphIcon::kMeasureRectangle:
        painter.drawRect(QRectF(6, 8, 20, 16));
        painter.drawLine(QPointF(6, 12), QPointF(10, 8));
        painter.drawLine(QPointF(22, 20), QPointF(26, 24));
        break;
    case GlyphIcon::kMeasureDistance:
        painter.drawLine(QPointF(7, 24), QPointF(25, 8));
        painter.drawRect(QRectF(5, 22, 4, 4));
        painter.drawRect(QRectF(23, 6, 4, 4));
        break;
    case GlyphIcon::kOptions:
        painter.drawEllipse(QPointF(16, 16), 9, 9);
        painter.drawLine(QPointF(16, 11), QPointF(16, 21));
        painter.drawLine(QPointF(11, 16), QPointF(21, 16));
        break;
    case GlyphIcon::kMinimize:
        painter.drawLine(QPointF(7, 20), QPointF(25, 20));
        break;
    case GlyphIcon::kAbout:
        painter.drawEllipse(QPointF(16, 16), 10, 10);
        {
            QFont font = painter.font();
            font.setBold(true);
            font.setPixelSize(15);
            painter.setFont(font);
            painter.drawText(QRectF(6, 5, 20, 22), Qt::AlignCenter, QStringLiteral("i"));
        }
        break;
    case GlyphIcon::kArrowSolid: {
        painter.setPen(QPen(color, 2.2, Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(16, 28), QPointF(16, 13));
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawPolygon(QPolygonF{QPointF(16, 4), QPointF(7, 14), QPointF(25, 14)});
        break;
    }
    case GlyphIcon::kArrowSolidTail:
    case GlyphIcon::kArrowHollowTail: {
        QPolygonF arrow;
        arrow << QPointF(16, 4) << QPointF(7, 14) << QPointF(13, 14) << QPointF(15, 28)
              << QPointF(17, 28) << QPointF(19, 14) << QPointF(25, 14);
        if (glyph == GlyphIcon::kArrowSolidTail) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawPolygon(arrow);
        } else {
            painter.setBrush(Qt::NoBrush);
            painter.drawPolygon(arrow);
        }
        break;
    }
    case GlyphIcon::kMosaicBrush:
        painter.drawLine(QPointF(7, 25), QPointF(23, 9));
        for (int index = 0; index < 6; ++index) {
            const QPointF center(8 + index * 3.0, 24 - index * 3.0);
            painter.fillRect(QRectF(center.x() - 1.2, center.y() - 1.2, 2.4, 2.4), color);
        }
        break;
    case GlyphIcon::kMosaicArea:
        painter.drawRect(QRectF(5, 7, 22, 18));
        for (int row = 0; row < 2; ++row) {
            for (int column = 0; column < 3; ++column) {
                if ((row + column) % 2 == 0) {
                    painter.fillRect(QRectF(8 + column * 6, 11 + row * 6, 4, 4), color);
                }
            }
        }
        break;
    case GlyphIcon::kBlur:
        for (const qreal radius : {4.0, 7.0, 10.0}) {
            QColor ring = color;
            ring.setAlphaF(1.0 - (radius - 4.0) / 10.0);
            painter.setPen(QPen(ring, 1.4));
            painter.drawEllipse(QPointF(16, 16), radius, radius);
        }
        break;
    }
}

QPixmap renderPixmap(GlyphIcon glyph, int size, const QColor &color)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.scale(qreal(size) / DESIGN_SIZE, qreal(size) / DESIGN_SIZE);
    drawGlyph(painter, glyph, color);
    return pixmap;
}
} // namespace

QIcon createGlyphIcon(GlyphIcon glyph)
{
    return createGlyphIcon(glyph, QColor(232, 232, 232));
}

QIcon createGlyphIcon(GlyphIcon glyph, const QColor &normal_color)
{
    QIcon icon;
    for (const int size : {16, 20, 24, 32}) {
        icon.addPixmap(renderPixmap(glyph, size, normal_color), QIcon::Normal);
        icon.addPixmap(renderPixmap(glyph, size, normal_color.lighter(125)), QIcon::Active);
        QColor disabled = normal_color;
        disabled.setAlpha(110);
        icon.addPixmap(renderPixmap(glyph, size, disabled), QIcon::Disabled);
    }
    return icon;
}

GlyphIcon applicationActionGlyph(ApplicationAction action)
{
    switch (action) {
    case ApplicationAction::kSnip:
        return GlyphIcon::kCaptureRegion;
    case ApplicationAction::kCaptureActiveWindow:
        return GlyphIcon::kActiveWindow;
    case ApplicationAction::kScrollSnip:
        return GlyphIcon::kScrollCapture;
    case ApplicationAction::kOcrSnip:
        return GlyphIcon::kOcr;
    case ApplicationAction::kPickColor:
        return GlyphIcon::kColorPicker;
    case ApplicationAction::kScreenRuler:
        return GlyphIcon::kScreenRuler;
    case ApplicationAction::kPreferences:
        return GlyphIcon::kPreferences;
    case ApplicationAction::kToggleCaptureBar:
        return GlyphIcon::kOptions;
    case ApplicationAction::kAbout:
        return GlyphIcon::kAbout;
    case ApplicationAction::kQuit:
        return GlyphIcon::kClose;
    default:
        return GlyphIcon::kCaptureRegion;
    }
}

QIcon createColorSwatchIcon(const QColor &color)
{
    QIcon icon;
    for (const int size : {16, 20, 24, 32}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(238, 238, 238), qMax(1.0, size / 16.0)));
        painter.setBrush(color);
        painter.drawRoundedRect(QRectF(3, 3, size - 6, size - 6), 3, 3);
        icon.addPixmap(pixmap);
    }
    return icon;
}
