#pragma once

#include <QColor>
#include <QStandardPaths>
#include <QString>

[[nodiscard]] inline QString defaultCaptureSaveDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

[[nodiscard]] inline QString resolveCaptureSaveDirectory(const QString &configuredPath)
{
    const QString path = configuredPath.trimmed();
    return path.isEmpty() ? defaultCaptureSaveDirectory() : path;
}

enum class CaptureOutputAction { kNone, kCopy, kPin, kSave, kQuickSave };

struct CaptureOptions
{
    bool                detectWindows{true};
    bool                detectUiElements{true};
    bool                showAnchors{true};
    bool                showTips{true};
    bool                hide1x1{false};
    bool                drawOutsideArea{true};
    bool                autoCrossLines{false};
    bool                askForConfirmOnEscape{false};
    bool                abortOnFocusOut{false};
    bool                loopHistoryAreas{true};
    bool                enterKeyOutput{true};
    bool                doubleClickOutput{true};
    bool                middleClickOutput{true};
    CaptureOutputAction enterAction{CaptureOutputAction::kCopy};
    CaptureOutputAction doubleClickAction{CaptureOutputAction::kCopy};
    CaptureOutputAction middleClickAction{CaptureOutputAction::kCopy};
    bool                enterExit{true};
    bool                doubleClickExit{true};
    bool                middleClickExit{true};
    bool                captureCursor{false};
    bool                neverCaptureCursor{false};
    bool                showMagnifier{true};
    bool                showMaskInMagnifier{true};
    bool                showBorderInMagnifier{true};
    bool                showAnchorsInMagnifier{true};
    bool                uppercaseHexColor{true};
    bool                physicalSelectionUnits{false};
    bool                disableMouseScroll{false};
    bool                autoSave{false};
    bool                soundEnabled{false};
    int                 maxHistoryAreas{30};
    int                 borderWidth{1};
    QColor              anchorBorderColor{0, 0, 0, 255};
    QString             quickSavePath{defaultCaptureSaveDirectory()};
    QString             manualSavePath{defaultCaptureSaveDirectory()};
    QString             autoSavePath{defaultCaptureSaveDirectory()};
    QString             filenamePattern{QStringLiteral("QSnapCraft_$yyyy-MM-dd_HH-mm-ss$")};
    QString             imageFormat{QStringLiteral("PNG")};
    QString             ocrLanguage;
    int                 jpegQuality{95};
};
