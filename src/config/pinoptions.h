#pragma once

#include <QColor>
#include <QString>

enum class TextImageFormat { kPlainText, kHtml };

struct TextToImageOptions
{
    QString fontFamily;
    int     fontSize{16};
    int     maxWidth{720};
    int     padding{16};
    QColor  textColor{0, 0, 0, 255};
    QColor  backgroundColor{255, 255, 255, 255};
};

struct PinSettingsData
{
    bool               showShadow{true};
    bool               stayOnTop{true};
    TextToImageOptions textToImage;
};

struct OcrSettingsData
{
    QString language;
};
