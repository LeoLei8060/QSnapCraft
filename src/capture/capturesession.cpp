#include "capturesession.h"

#include "output/outputservice.h"
#include "paint/paintscene.h"
#include "platform/windows/desktopcapture.h"
#include "platform/windows/foregroundwindow.h"
#include "platform/windows/ocrengine.h"
#include "platform/windows/shortcutmanager.h"
#include "ui/captureoverlay.h"
#include "ui/ocrdialog.h"
#include "ui/scrollcaptureindicator.h"

#include <QApplication>
#include <QCursor>
#include <QDebug>

namespace {
void notifyOutput(const CaptureOptions &options)
{
    if (options.soundEnabled) {
        QApplication::beep();
    }
}

QPoint selectionGlobalPosition(const QRect &selection, const QRect &logical_bounds)
{
    return selection.topLeft() + logical_bounds.topLeft();
}
} // namespace

CaptureSession::CaptureSession(QObject *parent)
    : QObject(parent)
{
    m_scrollTimer.setInterval(350);
    connect(&m_scrollTimer, &QTimer::timeout, this, &CaptureSession::captureScrollFrame);
}

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
    stopScrollCapture();
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
    if (m_request.intent == CaptureIntent::kScroll) {
        beginScrollCapture(m_overlay->currentSelection());
        return;
    }

    m_phase = CapturePhase::kAnnotating;
    m_overlay->enterAnnotating();
    if (m_request.intent == CaptureIntent::kOcr && showOcrDialog()) {
        const QRect         selection = m_overlay->currentSelection();
        const CaptureResult result{selection,
                                   selectionGlobalPosition(selection, m_frame.logicalBounds),
                                   {},
                                   CaptureIntent::kOcr};
        m_overlay->paintScene()->clearAll();
        m_overlay->hide();
        resetSession();
        emit sigFinished(result);
    }
}

void CaptureSession::beginScrollCapture(const QRect &selection)
{
    if (!selection.isValid() || !m_overlay) {
        fail(QStringLiteral("滚动截图选区无效。"));
        return;
    }

    m_scrollSelection = selection;
    m_scrollSelectionGlobal = selection.translated(m_frame.logicalBounds.topLeft());
    QImage  first_frame;
    QString error;
    if (!renderCaptureSelection(m_frame, m_scrollSelectionGlobal, &first_frame, &error)) {
        fail(error);
        return;
    }

    m_scrollStitcher.reset(first_frame);
    m_overlay->hide();
    if (!m_scrollIndicator) {
        m_scrollIndicator = std::make_unique<ScrollCaptureIndicator>();
    }
    m_scrollIndicator->setSelection(m_scrollSelectionGlobal);
    m_scrollIndicator->setCapturedSize(first_frame.size());
    m_phase = CapturePhase::kScrolling;
    m_scrollActive = true;

    if (!ShortcutManager::instance()->registerHotkey(QStringLiteral("Enter"), ShortcutManager::scrollFinishKey)) {
        emit sigOperationFailed(QStringLiteral("Enter 全局热键注册失败，可使用截图工具条完成滚动截图。"));
    }
    focusWindowUnderPoint(QCursor::pos());
    m_scrollTimer.start();
}

void CaptureSession::captureScrollFrame()
{
    if (!m_scrollActive) {
        return;
    }

    const CaptureFrame live_frame = captureVirtualDesktop(false);
    if (!live_frame.isValid()) {
        return;
    }

    QImage frame_image;
    if (!renderCaptureSelection(live_frame, m_scrollSelectionGlobal, &frame_image)) {
        return;
    }
    int added_rows = 0;
    m_scrollStitcher.appendFrame(frame_image, &added_rows);
    if (m_scrollIndicator) {
        m_scrollIndicator->setCapturedSize(m_scrollStitcher.image().size());
    }
}

void CaptureSession::finishScrollCapture()
{
    if (!m_scrollActive) {
        return;
    }

    captureScrollFrame();
    const QImage image = m_scrollStitcher.image();
    if (image.isNull()) {
        emit sigOperationFailed(QStringLiteral("滚动截图没有得到有效图像。"));
        return;
    }

    QString error;
    if (!OutputService::copyImage(image, &error)) {
        emit sigOperationFailed(error);
        return;
    }
    if (!m_request.outputPath.isEmpty()
        && !OutputService::saveImage(image, m_request.outputPath, m_options.imageFormat, m_options.jpegQuality, &error)) {
        emit sigOperationFailed(error);
        return;
    }
    QString autoSaveError;
    if (!saveAutoSnapshot(image, &autoSaveError)) {
        emit sigOperationFailed(autoSaveError);
    }

    const CaptureResult result{m_scrollSelection, m_scrollSelectionGlobal.topLeft(), image, CaptureIntent::kScroll};
    m_overlay->recordSelectionForHistory();
    notifyOutput(m_options);
    stopScrollCapture();
    resetSession();
    emit sigFinished(result);
}

void CaptureSession::stopScrollCapture()
{
    if (!m_scrollActive && !m_scrollTimer.isActive()) {
        return;
    }
    m_scrollActive = false;
    m_scrollTimer.stop();
    ShortcutManager::instance()->unregisterHotkey(ShortcutManager::scrollFinishKey);
    if (m_scrollIndicator) {
        m_scrollIndicator->hide();
    }
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
            const QRect         selection = m_overlay->currentSelection();
            const CaptureResult result{selection,
                                       selectionGlobalPosition(selection, m_frame.logicalBounds),
                                       m_overlay->renderSelection(),
                                       CaptureIntent::kPin};
            emit                sigPinned(result);
        }
        break;
    case CaptureCommand::kOcr:
        showOcrDialog();
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
        QString autoSaveError;
        if (!saveAutoSnapshot(image, &autoSaveError)) {
            emit sigOperationFailed(autoSaveError);
        }
        notifyOutput(m_options);
        if (closeAfter) {
            m_overlay->recordSelectionForHistory();
            m_phase = CapturePhase::kFinishing;
            const CaptureResult result{m_overlay->currentSelection(),
                                       selectionGlobalPosition(m_overlay->currentSelection(), m_frame.logicalBounds),
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
        QString autoSaveError;
        if (!saveAutoSnapshot(image, &autoSaveError)) {
            emit sigOperationFailed(autoSaveError);
        }
        notifyOutput(m_options);
        if (closeAfter) {
            m_overlay->recordSelectionForHistory();
            m_phase = CapturePhase::kFinishing;
            const CaptureResult result{m_overlay->currentSelection(),
                                       selectionGlobalPosition(m_overlay->currentSelection(), m_frame.logicalBounds),
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

bool CaptureSession::showOcrDialog()
{
    const QImage               image = m_overlay->renderSelection();
    const OcrRecognitionResult result = recognizeImageText(image, m_options.ocrLanguage);
    if (!result.success) {
        emit sigOperationFailed(result.error);
        return false;
    }
    OcrDialog dialog(result.text, m_overlay.get());
    dialog.exec();
    return true;
}

bool CaptureSession::saveAutoSnapshot(const QImage &image, QString *error)
{
    if (!m_options.autoSave) {
        return true;
    }

    const QString path = OutputService::saveSnapshot(image,
                                                     m_options.autoSavePath,
                                                     m_options.filenamePattern,
                                                     m_options.imageFormat,
                                                     m_options.jpegQuality,
                                                     error);
    return !path.isEmpty();
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

    QString autoSaveError;
    if (!saveAutoSnapshot(image, &autoSaveError)) {
        emit sigOperationFailed(autoSaveError);
    }

    notifyOutput(m_options);
    m_overlay->recordSelectionForHistory();
    m_overlay->paintScene()->clearAll();
    m_overlay->hide();
    const CaptureResult result{selection, selectionGlobalPosition(selection, m_frame.logicalBounds), image, intent};
    resetSession();
    emit sigFinished(result);
}

void CaptureSession::resetSession()
{
    stopScrollCapture();
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
