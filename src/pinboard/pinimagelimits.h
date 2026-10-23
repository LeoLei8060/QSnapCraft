#pragma once

#include <QImage>
#include <QSize>

inline constexpr int    MAX_PIN_IMAGE_DIMENSION = 16384;
inline constexpr qint64 MAX_PIN_IMAGE_PIXELS = 64LL * 1024 * 1024;

bool   canCreatePinImage(const QSize &size);
QSize  boundedPinImageSize(const QSize &size);
QImage limitPinImageSize(const QImage &image);
