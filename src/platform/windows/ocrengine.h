#pragma once

#include <QImage>
#include <QString>

struct OcrRecognitionResult
{
    QString text;
    QString error;
    bool    success{false};
};

bool                 isOcrEngineAvailable();
OcrRecognitionResult recognizeImageText(const QImage &image, const QString &language = {});
