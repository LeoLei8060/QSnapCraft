#include "ui/rulermeasurement.h"

#include <QtTest>

class RulerMeasurementTest : public QObject
{
    Q_OBJECT

private slots:
    void measuresInclusiveSpans();
    void measuresDistanceWithZeroAxisRule();
    void formatsPixelsCentimetersAndInches();
};

void RulerMeasurementTest::measuresInclusiveSpans()
{
    QCOMPARE(rulerSpanPixels(20, 20), 0);
    QCOMPARE(rulerSpanPixels(20, 21), 2);
    QCOMPARE(rulerSpanPixels(20, 25), 6);
    QCOMPARE(rulerSpanPixels(25, 20), 6);
}

void RulerMeasurementTest::measuresDistanceWithZeroAxisRule()
{
    QCOMPARE(rulerDistancePixels(QPoint(10, 10), QPoint(210, 210)), 284);
    QCOMPARE(rulerDistancePixels(QPoint(10, 10), QPoint(210, 10)), 201);
    QCOMPARE(rulerDistancePixels(QPoint(10, 10), QPoint(10, 10)), 0);
}

void RulerMeasurementTest::formatsPixelsCentimetersAndInches()
{
    QCOMPARE(formatRulerMeasurement(201, RulerUnit::kPixels), QStringLiteral("201 px"));
    QCOMPARE(formatRulerMeasurement(201, RulerUnit::kCentimeters), QStringLiteral("5.3 cm"));
    QCOMPARE(formatRulerMeasurement(201, RulerUnit::kInches), QStringLiteral("2.09 in"));
}

QTEST_APPLESS_MAIN(RulerMeasurementTest)

#include "rulermeasurementtest.moc"
