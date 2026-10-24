#include "capture/capturesession.h"
#include "capture/captureoptions.h"
#include "platform/windows/ocrengine.h"
#include "ui/captureoverlay.h"
#include "ui/ocrdialog.h"

#include <QtTest>

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QToolButton>

class CaptureSessionTest : public QObject
{
    Q_OBJECT

private slots:
    void selectionStartsAnnotationWithoutCrash();
    void closeButtonCancelsActiveWindowAnnotation();
    void pinResultUsesSelectionGlobalPosition();
    void quickSaveAlsoWritesAutoSaveSnapshot();
    void reportsAutoSaveFailureWithoutDiscardingPrimaryOutput();
    void ocrIntentCompletesAfterResultDialog();
};

namespace {
CaptureOptions testOptions()
{
    CaptureOptions options;
    options.detectWindows = false;
    options.showMagnifier = false;
    options.showTips = false;
    options.showAnchors = false;
    return options;
}

CaptureOverlay *findOverlay()
{
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *overlay = dynamic_cast<CaptureOverlay *>(widget)) {
            return overlay;
        }
    }
    return nullptr;
}
} // namespace

void CaptureSessionTest::selectionStartsAnnotationWithoutCrash()
{
    CaptureSession session;
    CaptureRequest request;
    request.intent = CaptureIntent::kAnnotate;
    request.startMode = CaptureStartMode::kPoint;
    QVERIFY(session.start(request, testOptions()));

    CaptureOverlay *overlay = findOverlay();
    QVERIFY(overlay);
    QCOMPARE(session.phase(), CapturePhase::kSelecting);

    QTest::mousePress(overlay->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseRelease(overlay->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(320, 240));
    QTRY_COMPARE_WITH_TIMEOUT(session.phase(), CapturePhase::kAnnotating, 1000);
    QVERIFY(overlay->currentSelection().isValid());
    session.cancel();
}

void CaptureSessionTest::closeButtonCancelsActiveWindowAnnotation()
{
    CaptureSession session;
    CaptureRequest request;
    request.intent = CaptureIntent::kAnnotate;
    request.startMode = CaptureStartMode::kTwoCorners;
    request.initialSelection = QRect(100, 100, 500, 350);
    QSignalSpy cancelled(&session, &CaptureSession::sigCancelled);
    QVERIFY(session.start(request, testOptions()));
    QCOMPARE(session.phase(), CapturePhase::kAnnotating);

    CaptureOverlay *overlay = findOverlay();
    QVERIFY(overlay);
    QToolButton *close = overlay->findChild<QToolButton *>(QStringLiteral("btn_Close"));
    QVERIFY(close);
    QTest::mouseClick(close, Qt::LeftButton);

    QTRY_COMPARE_WITH_TIMEOUT(cancelled.count(), 1, 1000);
    QCOMPARE(session.phase(), CapturePhase::kIdle);
    QVERIFY(!overlay->isVisible());
}

void CaptureSessionTest::pinResultUsesSelectionGlobalPosition()
{
    CaptureSession session;
    CaptureRequest request;
    request.intent = CaptureIntent::kAnnotate;
    request.startMode = CaptureStartMode::kTwoCorners;
    request.initialSelection = QRect(100, 100, 500, 350);

    bool          finished = false;
    CaptureResult result;
    connect(&session, &CaptureSession::sigFinished, this, [&](const CaptureResult &captured_result) {
        result = captured_result;
        finished = true;
    });

    QVERIFY(session.start(request, testOptions()));
    CaptureOverlay *overlay = findOverlay();
    QVERIFY(overlay);
    QCOMPARE(session.phase(), CapturePhase::kAnnotating);

    const QPoint expected_position = overlay->geometry().topLeft() + overlay->currentSelection().topLeft();
    QKeyEvent    pin_event(QEvent::KeyPress, Qt::Key_T, Qt::ControlModifier);
    QCoreApplication::sendEvent(overlay, &pin_event);

    QTRY_VERIFY_WITH_TIMEOUT(finished, 1000);
    QCOMPARE(result.intent, CaptureIntent::kPin);
    QCOMPARE(result.globalPosition, expected_position);
}

void CaptureSessionTest::quickSaveAlsoWritesAutoSaveSnapshot()
{
    QTemporaryDir quickSaveDirectory;
    QTemporaryDir autoSaveDirectory;
    QVERIFY(quickSaveDirectory.isValid());
    QVERIFY(autoSaveDirectory.isValid());

    CaptureOptions options = testOptions();
    options.autoSave = true;
    options.quickSavePath = quickSaveDirectory.path();
    options.autoSavePath = autoSaveDirectory.path();
    options.filenamePattern = QStringLiteral("Capture");
    options.imageFormat = QStringLiteral("PNG");

    CaptureSession session;
    CaptureRequest request;
    request.intent = CaptureIntent::kAnnotate;
    request.startMode = CaptureStartMode::kTwoCorners;
    request.initialSelection = QRect(100, 100, 320, 240);
    QSignalSpy finished(&session, &CaptureSession::sigFinished);

    QVERIFY(session.start(request, options));
    CaptureOverlay *overlay = findOverlay();
    QVERIFY(overlay);
    QKeyEvent quickSaveEvent(QEvent::KeyPress, Qt::Key_S, Qt::ControlModifier | Qt::ShiftModifier);
    QCoreApplication::sendEvent(overlay, &quickSaveEvent);

    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 1000);
    QVERIFY(QFileInfo::exists(quickSaveDirectory.filePath(QStringLiteral("Capture.png"))));
    QVERIFY(QFileInfo::exists(autoSaveDirectory.filePath(QStringLiteral("Capture.png"))));
}

void CaptureSessionTest::reportsAutoSaveFailureWithoutDiscardingPrimaryOutput()
{
    QTemporaryDir quickSaveDirectory;
    QTemporaryDir autoSaveDirectory;
    QVERIFY(quickSaveDirectory.isValid());
    QVERIFY(autoSaveDirectory.isValid());

    QFile blocker(autoSaveDirectory.filePath(QStringLiteral("blocked")));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.write("not a directory");
    blocker.close();

    CaptureOptions options = testOptions();
    options.autoSave = true;
    options.quickSavePath = quickSaveDirectory.path();
    options.autoSavePath = blocker.fileName();
    options.filenamePattern = QStringLiteral("Capture");
    options.imageFormat = QStringLiteral("PNG");

    CaptureSession session;
    CaptureRequest request;
    request.intent = CaptureIntent::kAnnotate;
    request.startMode = CaptureStartMode::kTwoCorners;
    request.initialSelection = QRect(100, 100, 320, 240);
    QSignalSpy finished(&session, &CaptureSession::sigFinished);
    QSignalSpy operationFailed(&session, &CaptureSession::sigOperationFailed);

    QVERIFY(session.start(request, options));
    CaptureOverlay *overlay = findOverlay();
    QVERIFY(overlay);
    QKeyEvent quickSaveEvent(QEvent::KeyPress, Qt::Key_S, Qt::ControlModifier | Qt::ShiftModifier);
    QCoreApplication::sendEvent(overlay, &quickSaveEvent);

    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 1000);
    QVERIFY(QFileInfo::exists(quickSaveDirectory.filePath(QStringLiteral("Capture.png"))));
    QCOMPARE(operationFailed.count(), 1);
}

void CaptureSessionTest::ocrIntentCompletesAfterResultDialog()
{
    if (!isOcrEngineAvailable()) {
        QSKIP("Windows OCR language engine is unavailable");
    }

    CaptureSession session;
    CaptureRequest request;
    request.intent = CaptureIntent::kOcr;
    request.startMode = CaptureStartMode::kTwoCorners;
    request.initialSelection = QRect(50, 50, 420, 180);

    bool          finished = false;
    CaptureIntent finished_intent = CaptureIntent::kAnnotate;
    connect(&session, &CaptureSession::sigFinished, this, [&](const CaptureResult &result) {
        finished = true;
        finished_intent = result.intent;
    });
    QTimer dialog_timer;
    dialog_timer.setInterval(20);
    connect(&dialog_timer, &QTimer::timeout, this, [&dialog_timer]() {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            if (auto *dialog = qobject_cast<OcrDialog *>(widget)) {
                dialog_timer.stop();
                dialog->accept();
                return;
            }
        }
    });
    dialog_timer.start();

    QVERIFY(session.start(request, testOptions()));
    QVERIFY(finished);
    QCOMPARE(finished_intent, CaptureIntent::kOcr);
    QCOMPARE(session.phase(), CapturePhase::kIdle);
}

QTEST_MAIN(CaptureSessionTest)

#include "capturesessiontest.moc"
