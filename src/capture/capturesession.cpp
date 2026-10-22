#include "capturesession.h"

#include "platform/windows/desktopcapture.h"
#include "output/outputservice.h"
#include "paint/paintscene.h"
#include "ui/captureoverlay.h"

#include <QApplication>
#include <QDebug>

namespace {
void notifyOutput(const CaptureOptions &options)
{
    if (options.soundEnabled) {
        QApplication::beep();
    }
}
} // namespace

CaptureSession::CaptureSession(QObject *parent)
    : QObject(parent)
{}

CaptureSession::~CaptureSession() = default;

bool CaptureSession::start(const CaptureRequest &request, const CaptureOptions &options)
{
    if (m_phase != CapturePhase::kIdle) {
        return false;
    }

    m_phase = CapturePhase::kStarting;
    m_request = request;
    m_options = options;
    const bool includeCursor = options.captureCursor && !options.neverCaptureCursor;
    m_frame = captureVirtualDesktop(includeCursor);
    if (!m_frame.isValid()) {
        fail(m_frame.error);
        return false;
    }

    if (!m_overlay) {
        m_overlay = std::make_unique<CaptureOverlay>();
        connect(m_overlay.get(), &CaptureOverlay::sigSelectionConfirmed, this, &CaptureSession::onSelectionConfirmed);
        connect(m_overlay.get(), &CaptureOverlay::sigCommandRequested, this, &CaptureSession::onCommandRequested);
        connect(m_overlay.get(), &CaptureOverlay::sigCancelRequested, this, &CaptureSession::onCancelRequested);
    }

    emit sigStarted();
    m_overlay->begin(m_frame, m_options, m_request);
    m_phase = CapturePhase::kSelecting;
    if (m_request.startMode == CaptureStartMode::kTwoCorners && m_overlay->currentSelection().isValid()) {
        onSelectionConfirmed(m_overlay->currentSelection());
    }
    return true;
}

void CaptureSession::cancel()
{
    if (m_phase == CapturePhase::kIdle) {
        return;
    }
    if (m_overlay) {
        m_overlay->paintScene()->clearAll();
        m_overlay->hide();
    }
    resetSession();
    emit sigCancelled();
}

bool CaptureSession::isActive() const
{
    return m_phase != CapturePhase::kIdle;
}

CapturePhase CaptureSession::phase() const
{
    return m_phase;
}

void CaptureSession::onSelectionConfirmed(const QRect &selection)
{
    if (selection.isValid()) {
        m_overlay->setSelection(selection);
    }
    if (m_request.intent == CaptureIntent::kCopy || m_request.intent == CaptureIntent::kPin) {
        finishWithIntent(m_request.intent, m_request.intent == CaptureIntent::kCopy);
        return;
    }

    m_phase = CapturePhase::kAnnotating;
    m_overlay->enterAnnotating();
}

void CaptureSession::onCommandRequested(CaptureCommand command, bool closeAfter)
{
    switch (command) {
    case CaptureCommand::kFinish:
        finishWithIntent(CaptureIntent::kCopy, true);
        break;
    case CaptureCommand::kCopy:
        if (closeAfter) {
            finishWithIntent(CaptureIntent::kCopy, true);
        } else {
            QString error;
            if (!OutputService::copyImage(m_overlay->renderSelection(), &error)) {
                emit sigFailed(error);
            } else {
                notifyOutput(m_options);
            }
        }
        break;
    case CaptureCommand::kPin:
        if (closeAfter) {
            finishWithIntent(CaptureIntent::kPin, false);
        } else {
            emit sigPinned(CaptureResult{m_overlay->currentSelection(),
                                         m_frame.logicalBounds.topLeft(),
                                         m_overlay->renderSelection(),
                                         CaptureIntent::kPin});
        }
        break;
    case CaptureCommand::kSave: {
        const QImage image = m_overlay->renderSelection();
        if (image.isNull()) {
            break;
        }
        const QString path = OutputService::chooseSavePath(m_overlay.get(), m_options, image);
        if (path.isEmpty()) {
            break;
        }
        QString error;
        if (!OutputService::saveImage(image, path, m_options.imageFormat, m_options.jpegQuality, &error)) {
            emit sigFailed(error);
            break;
        }
        notifyOutput(m_options);
        if (closeAfter) {
            m_overlay->recordSelectionForHistory();
            m_phase = CapturePhase::kFinishing;
            const CaptureResult result{m_overlay->currentSelection(),
                                       m_frame.logicalBounds.topLeft(),
                                       image,
                                       CaptureIntent::kAnnotate};
            m_overlay->paintScene()->clearAll();
            m_overlay->hide();
            resetSession();
            emit sigFinished(result);
        }
        break;
    }
    case CaptureCommand::kQuickSave: {
        const QImage  image = m_overlay->renderSelection();
        QString       error;
        const QString path = OutputService::quickSave(image, m_options, &error);
        if (path.isEmpty()) {
            emit sigFailed(error);
            break;
        }
        notifyOutput(m_options);
        if (closeAfter) {
            m_overlay->recordSelectionForHistory();
            m_phase = CapturePhase::kFinishing;
            const CaptureResult result{m_overlay->currentSelection(),
                                       m_frame.logicalBounds.topLeft(),
                                       image,
                                       CaptureIntent::kAnnotate};
            m_overlay->paintScene()->clearAll();
            m_overlay->hide();
            resetSession();
            emit sigFinished(result);
        }
        break;
    }
    case CaptureCommand::kCancel:
        cancel();
        break;
    default:
        break;
    }
}

void CaptureSession::onCancelRequested()
{
    cancel();
}

void CaptureSession::finishWithIntent(CaptureIntent intent, bool copyImage)
{
    if (m_phase == CapturePhase::kIdle || !m_overlay) {
        return;
    }

    m_phase = CapturePhase::kFinishing;
    const QRect  selection = m_overlay->currentSelection();
    const QImage image = m_overlay->renderSelection();
    if (image.isNull()) {
        fail(QStringLiteral("无法生成截图输出。"));
        return;
    }

    QString error;
    if (copyImage && !OutputService::copyImage(image, &error)) {
        emit sigFailed(error);
        m_phase = CapturePhase::kAnnotating;
        return;
    }
    if (!m_request.outputPath.isEmpty()
        && !OutputService::saveImage(image, m_request.outputPath, m_options.imageFormat, m_options.jpegQuality, &error)) {
        emit sigFailed(error);
        m_phase = CapturePhase::kAnnotating;
        return;
    }

    if (m_options.autoSave) {
        QString       autoSaveError;
        const QString autoSavePath = OutputService::saveSnapshot(image,
                                                                 m_options.autoSavePath,
                                                                 m_options.filenamePattern,
                                                                 m_options.imageFormat,
                                                                 m_options.jpegQuality,
                                                                 &autoSaveError);
        if (autoSavePath.isEmpty()) {
            qWarning() << autoSaveError;
        }
    }

    notifyOutput(m_options);
    m_overlay->recordSelectionForHistory();
    m_overlay->paintScene()->clearAll();
    m_overlay->hide();
    const CaptureResult result{selection, m_frame.logicalBounds.topLeft(), image, intent};
    resetSession();
    emit sigFinished(result);
}

void CaptureSession::resetSession()
{
    m_phase = CapturePhase::kIdle;
    m_request = {};
    m_frame = {};
}

void CaptureSession::fail(const QString &error)
{
    if (m_overlay) {
        m_overlay->paintScene()->clearAll();
        m_overlay->hide();
    }
    resetSession();
    emit sigFailed(error);
}
