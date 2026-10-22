#include "capture/capturesession.h"
#include "capture/captureoptions.h"
#include "ui/captureoverlay.h"

#include <QtTest>

#include <QApplication>
#include <QToolButton>

class CaptureSessionTest : public QObject
{
    Q_OBJECT

private slots:
    void selectionStartsAnnotationWithoutCrash();
    void closeButtonCancelsActiveWindowAnnotation();
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

QTEST_MAIN(CaptureSessionTest)

#include "capturesessiontest.moc"
