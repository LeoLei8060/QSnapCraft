#include "pinimagelimits.h"

#include <QtMath>

#include <algorithm>

bool canCreatePinImage(const QSize &size)
{
    if (!size.isValid()) {
        return false;
    }
    return size.width() <= MAX_PIN_IMAGE_DIMENSION && size.height() <= MAX_PIN_IMAGE_DIMENSION
           && qint64(size.width()) * size.height() <= MAX_PIN_IMAGE_PIXELS;
}

QSize boundedPinImageSize(const QSize &size)
{
    if (!size.isValid()) {
        return {};
    }
    if (canCreatePinImage(size)) {
        return size;
    }

    qreal scale = qMin(qreal(MAX_PIN_IMAGE_DIMENSION) / size.width(), qreal(MAX_PIN_IMAGE_DIMENSION) / size.height());
    const qreal pixelScale = std::sqrt(qreal(MAX_PIN_IMAGE_PIXELS) / (qint64(size.width()) * size.height()));
    scale = qMin(scale, pixelScale);

    return QSize(qMax(1, qFloor(size.width() * scale)), qMax(1, qFloor(size.height() * scale)));
}

QImage limitPinImageSize(const QImage &image)
{
    if (image.isNull()) {
        return {};
    }
    const QSize boundedSize = boundedPinImageSize(image.size());
    if (boundedSize == image.size()) {
        return image;
    }

    QImage bounded = image.scaled(boundedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    bounded.setDevicePixelRatio(image.devicePixelRatio());
    return bounded;
}
