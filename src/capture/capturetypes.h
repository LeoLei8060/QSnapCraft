#pragma once

#include <QImage>
#include <QRect>
#include <QString>
#include <QVector>

enum class CapturePhase { kIdle, kStarting, kSelecting, kAnnotating, kFinishing };

enum class CaptureIntent { kAnnotate, kCopy, kPin };

enum class CaptureStartMode { kPoint, kFullDesktop, kLastArea, kCenteredSize, kActiveScreen, kTwoCorners };

enum class CaptureCommand { kCopy, kSave, kQuickSave, kPin, kUndo, kRedo, kDelete, kClearAll, kFinish, kCancel };

struct CaptureRequest
{
    CaptureIntent    intent{CaptureIntent::kAnnotate};
    CaptureStartMode startMode{CaptureStartMode::kPoint};
    QRect            initialSelection;
    QSize            centeredSize;
    QString          outputPath;
    bool             silentOutput{false};
};

struct CaptureResult
{
    QRect         logicalSelection;
    QPoint        globalPosition;
    QImage        image;
    CaptureIntent intent{CaptureIntent::kAnnotate};
};

struct DisplaySurface
{
    QString name;
    QRect   logicalGeometry;
    QImage  nativeImage;
    qreal   devicePixelRatio{1.0};
};

struct CaptureFrame
{
    QRect                   logicalBounds;
    QVector<DisplaySurface> displays;
    QImage                  logicalComposite;
    QString                 error;

    bool isValid() const
    {
        return error.isEmpty() && logicalBounds.isValid() && !displays.isEmpty() && !logicalComposite.isNull();
    }
};

bool renderCaptureSelection(const CaptureFrame &frame,
                            const QRect        &logicalSelection,
                            QImage             *image,
                            QString            *error = nullptr);
