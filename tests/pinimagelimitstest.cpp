#include "pinboard/pinimagelimits.h"

#include <QtTest>

class PinImageLimitsTest : public QObject
{
    Q_OBJECT

private slots:
    void preservesSupportedSize();
    void limitsOversizedDimension();
};

void PinImageLimitsTest::preservesSupportedSize()
{
    const QSize size(1920, 1080);
    QCOMPARE(boundedPinImageSize(size), size);
    QVERIFY(canCreatePinImage(size));
}

void PinImageLimitsTest::limitsOversizedDimension()
{
    const QSize size(20000, 200);
    const QSize bounded = boundedPinImageSize(size);
    QVERIFY(bounded.width() <= MAX_PIN_IMAGE_DIMENSION);
    QVERIFY(bounded.height() <= MAX_PIN_IMAGE_DIMENSION);
    QVERIFY(canCreatePinImage(bounded));
}

QTEST_APPLESS_MAIN(PinImageLimitsTest)

#include "pinimagelimitstest.moc"
