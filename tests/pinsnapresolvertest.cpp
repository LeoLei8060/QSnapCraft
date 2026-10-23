#include "pinboard/pinsnapresolver.h"

#include <QtTest>

class PinSnapResolverTest : public QObject
{
    Q_OBJECT

private slots:
    void snapsToScreenEdgeWithinThreshold();
    void snapsToNearbyPinEdge();
    void leavesGeometryAloneOutsideThreshold();
};

void PinSnapResolverTest::snapsToScreenEdgeWithinThreshold()
{
    const PinSnapResult result = PinSnapResolver::resolve(QRect(7, 120, 100, 80), QRect(0, 0, 800, 600), {}, 8);

    QCOMPARE(result.contentRect, QRect(0, 120, 100, 80));
    QCOMPARE(result.guides.size(), 1);
    QCOMPARE(result.guides.constFirst().x1(), 0.0);
    QCOMPARE(result.guides.constFirst().x2(), 0.0);
}

void PinSnapResolverTest::snapsToNearbyPinEdge()
{
    const QRect         target(300, 140, 120, 90);
    const PinSnapResult result = PinSnapResolver::resolve(QRect(194, 130, 100, 80), QRect(0, 0, 800, 600), {target}, 8);

    QCOMPARE(result.contentRect, QRect(200, 130, 100, 80));
    QCOMPARE(result.guides.size(), 1);
    QCOMPARE(result.guides.constFirst().x1(), 300.0);
    QCOMPARE(result.guides.constFirst().x2(), 300.0);
}

void PinSnapResolverTest::leavesGeometryAloneOutsideThreshold()
{
    const QRect         moving(250, 200, 100, 80);
    const PinSnapResult result = PinSnapResolver::resolve(moving, QRect(0, 0, 800, 600), {QRect(400, 200, 100, 80)}, 8);

    QCOMPARE(result.contentRect, moving);
    QVERIFY(result.guides.isEmpty());
}

QTEST_APPLESS_MAIN(PinSnapResolverTest)

#include "pinsnapresolvertest.moc"
