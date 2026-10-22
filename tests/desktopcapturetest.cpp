#include "platform/windows/desktopcapture.h"

#include <QtTest>

class DesktopCaptureTest : public QObject
{
    Q_OBJECT

private slots:
    void capturesVirtualDesktopImage();
    void rendersSelectionAtNativeResolution();
    void rendersMixedDpiSelectionWithoutGaps();
};

void DesktopCaptureTest::capturesVirtualDesktopImage()
{
    const CaptureFrame frame = captureVirtualDesktop();

    QVERIFY2(frame.isValid(), qPrintable(frame.error));
    QVERIFY(frame.logicalBounds.width() > 0);
    QVERIFY(frame.logicalBounds.height() > 0);
    QCOMPARE(frame.logicalComposite.size(), frame.logicalBounds.size());
    QVERIFY(!frame.logicalComposite.isNull());
}

void DesktopCaptureTest::rendersSelectionAtNativeResolution()
{
    QImage leftImage(200, 100, QImage::Format_ARGB32);
    leftImage.fill(Qt::red);
    QImage rightImage(400, 200, QImage::Format_ARGB32);
    rightImage.fill(Qt::blue);

    CaptureFrame frame;
    frame.logicalBounds = QRect(-100, 0, 300, 100);
    frame.displays = {
        DisplaySurface{QStringLiteral("left"), QRect(-100, 0, 100, 100), leftImage, 2.0},
        DisplaySurface{QStringLiteral("right"), QRect(0, 0, 200, 100), rightImage, 2.0},
    };
    frame.logicalComposite = QImage(300, 100, QImage::Format_ARGB32);
    frame.logicalComposite.fill(Qt::black);

    QImage  output;
    QString error;
    QVERIFY(renderCaptureSelection(frame, QRect(-50, 10, 100, 60), &output, &error));
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(output.size(), QSize(200, 120));
    QCOMPARE(output.pixelColor(10, 10), QColor(Qt::red));
    QCOMPARE(output.pixelColor(150, 10), QColor(Qt::blue));
}

void DesktopCaptureTest::rendersMixedDpiSelectionWithoutGaps()
{
    QImage leftImage(100, 100, QImage::Format_ARGB32);
    leftImage.fill(Qt::red);
    QImage rightImage(400, 200, QImage::Format_ARGB32);
    rightImage.fill(Qt::blue);

    CaptureFrame frame;
    frame.logicalBounds = QRect(-100, 0, 300, 100);
    frame.displays = {
        DisplaySurface{QStringLiteral("left"), QRect(-100, 0, 100, 100), leftImage, 1.0},
        DisplaySurface{QStringLiteral("right"), QRect(0, 0, 200, 100), rightImage, 2.0},
    };
    frame.logicalComposite = QImage(300, 100, QImage::Format_ARGB32);
    frame.logicalComposite.fill(Qt::black);

    QImage output;
    QVERIFY(renderCaptureSelection(frame, QRect(-50, 0, 150, 100), &output));
    QCOMPARE(output.size(), QSize(300, 200));
    for (int x = 0; x < output.width(); ++x) {
        QVERIFY2(output.pixelColor(x, 100).alpha() == 255, qPrintable(QString::number(x)));
    }
    QCOMPARE(output.pixelColor(25, 100), QColor(Qt::red));
    QCOMPARE(output.pixelColor(275, 100), QColor(Qt::blue));
}

QTEST_MAIN(DesktopCaptureTest)

#include "desktopcapturetest.moc"
