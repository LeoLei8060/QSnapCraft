#pragma once

#include "capture/captureoptions.h"
#include "capture/capturetypes.h"

#include <memory>
#include <QObject>

class CaptureOverlay;

class CaptureSession : public QObject
{
    Q_OBJECT

public:
    explicit CaptureSession(QObject *parent = nullptr);
    ~CaptureSession() override;

    bool         start(const CaptureRequest &request, const CaptureOptions &options);
    void         cancel();
    bool         isActive() const;
    CapturePhase phase() const;

signals:
    void sigStarted();
    void sigFinished(const CaptureResult &result);
    void sigPinned(const CaptureResult &result);
    void sigCancelled();
    void sigFailed(const QString &error);

private:
    void onSelectionConfirmed(const QRect &selection);
    void onCommandRequested(CaptureCommand command, bool closeAfter);
    void onCancelRequested();
    void finishWithIntent(CaptureIntent intent, bool copyImage);
    void resetSession();
    void fail(const QString &error);

    std::unique_ptr<CaptureOverlay> m_overlay;
    CaptureRequest                  m_request;
    CaptureOptions                  m_options;
    CaptureFrame                    m_frame;
    CapturePhase                    m_phase{CapturePhase::kIdle};
};
