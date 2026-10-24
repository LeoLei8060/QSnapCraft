#pragma once

#include <QColor>
#include <QString>

enum class TextImageFormat { kPlainText, kHtml };

struct PinGroupData
{
    QString id;
    QString name;
};

enum class PinGroupAction { kShow, kHide, kClose, kRename, kRemove };

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
    int                recoveryLimit{20};
    int                snapDistancePx{8};
    bool               smoothScaling{true};
    bool               zoomAtCursor{true};
    TextToImageOptions textToImage;
};

struct OcrSettingsData
{
    QString language;
};
