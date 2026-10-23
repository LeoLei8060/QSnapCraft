#pragma once

#include "config/pinoptions.h"

#include <QImage>
#include <QString>

QImage renderTextToImage(const QString &text, TextImageFormat format, const TextToImageOptions &options);
