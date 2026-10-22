#pragma once

#include "capture/captureoptions.h"

#include <QImage>
#include <QString>

class QWidget;

class OutputService
{
public:
    static bool copyImage(const QImage &image, QString *error = nullptr);
    static bool saveImage(
        const QImage &image, const QString &path, const QString &format, int quality, QString *error = nullptr);
    static QString quickSave(const QImage &image, const CaptureOptions &options, QString *error = nullptr);
    static QString saveSnapshot(const QImage  &image,
                                const QString &directory,
                                const QString &pattern,
                                const QString &format,
                                int            quality,
                                QString       *error = nullptr);
    static QString chooseSavePath(QWidget *parent, const CaptureOptions &options, const QImage &image);
};
