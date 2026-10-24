#pragma once

#include <QImage>
#include <QString>
#include <QStringList>

struct OcrRecognitionResult
{
    QString text;
    QString error;
    bool    success{false};
};

bool                 isOcrEngineAvailable();
QStringList          availableOcrLanguages();
OcrRecognitionResult recognizeImageText(const QImage &image, const QString &language = {});
