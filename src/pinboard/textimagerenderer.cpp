#include "textimagerenderer.h"

#include "pinboard/pinimagelimits.h"

#include <QAbstractTextDocumentLayout>
#include <QFont>
#include <QPainter>
#include <QPalette>
#include <QTextDocument>
#include <QtMath>

QImage renderTextToImage(const QString &text, TextImageFormat format, const TextToImageOptions &options)
{
    if (text.trimmed().isEmpty()) {
        return {};
    }

    const int padding = qMax(0, options.padding);
    const int max_width = qMax(64, options.maxWidth);
    const int content_width = qMax(1, max_width - padding * 2);

    QTextDocument document;
    QFont         font(options.fontFamily);
    font.setPixelSize(qMax(6, options.fontSize));
    document.setDefaultFont(font);
    if (format == TextImageFormat::kHtml) {
        document.setHtml(text);
    } else {
        document.setPlainText(text);
    }
    document.setTextWidth(content_width);

    const QSizeF document_size = document.documentLayout()->documentSize();
    const int    width = qMin(max_width, qMax(1, qCeil(document_size.width())) + padding * 2);
    const int    height = qMax(1, qCeil(document_size.height())) + padding * 2;
    if (!canCreatePinImage(QSize(width, height))) {
        return {};
    }

    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(options.backgroundColor);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.translate(padding, padding);
    QAbstractTextDocumentLayout::PaintContext context;
    context.palette.setColor(QPalette::Text, options.textColor);
    document.documentLayout()->draw(&painter, context);
    return image;
}
