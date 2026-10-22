#include "ui/capturebargeometry.h"

#include <QtTest>

class CaptureBarGeometryTest : public QObject
{
    Q_OBJECT

private slots:
    void snapsToRightEdge();
    void createsHorizontalHotspotForTopDock();
    void remainsFloatingOutsideSnapDistance();
    void preservesNegativeMonitorOrigin();
    void mapsDockEdgeNames();
    void clampsBarInsideScreen();
    void clampsHotspotInsideRightEdge();
};

void CaptureBarGeometryTest::snapsToRightEdge()
{
    const DockPlacement placement = calculateDockPlacement(QRect(0, 0, 1920, 1040),
                                                           QSize(48, 320),
                                                           QPoint(1860, 360),
                                                           12,
                                                           4);

    QCOMPARE(static_cast<int>(placement.edge), static_cast<int>(DockEdge::Right));
    QCOMPARE(placement.barRect.right(), 1919);
    QCOMPARE(placement.hotspotRect, QRect(1916, 360, 4, 50));
}

void CaptureBarGeometryTest::createsHorizontalHotspotForTopDock()
{
    const DockPlacement placement = calculateDockPlacement(QRect(0, 0, 1920, 1080),
                                                           QSize(320, 48),
                                                           QPoint(700, 4),
                                                           12,
                                                           4);

    QCOMPARE(static_cast<int>(placement.edge), static_cast<int>(DockEdge::Top));
    QCOMPARE(placement.hotspotRect, QRect(700, 0, 50, 4));
}

void CaptureBarGeometryTest::remainsFloatingOutsideSnapDistance()
{
    const DockPlacement placement = calculateDockPlacement(QRect(0, 0, 1920, 1080),
                                                           QSize(48, 320),
                                                           QPoint(700, 400),
                                                           12,
                                                           4);

    QCOMPARE(static_cast<int>(placement.edge), static_cast<int>(DockEdge::None));
    QCOMPARE(placement.barRect.topLeft(), QPoint(700, 400));
}

void CaptureBarGeometryTest::preservesNegativeMonitorOrigin()
{
    const DockPlacement placement = calculateDockPlacement(QRect(-1920, -200, 1920, 1080),
                                                           QSize(48, 320),
                                                           QPoint(-1920, 80),
                                                           12,
                                                           4);

    QCOMPARE(static_cast<int>(placement.edge), static_cast<int>(DockEdge::Left));
    QCOMPARE(placement.barRect.left(), -1920);
    QCOMPARE(placement.hotspotRect, QRect(-1920, 80, 4, 50));
}

void CaptureBarGeometryTest::mapsDockEdgeNames()
{
    QCOMPARE(dockEdgeName(DockEdge::Left), QStringLiteral("Left"));
    QCOMPARE(dockEdgeName(DockEdge::Right), QStringLiteral("Right"));
    QCOMPARE(static_cast<int>(dockEdgeFromName(QStringLiteral("Top"))), static_cast<int>(DockEdge::Top));
    QCOMPARE(static_cast<int>(dockEdgeFromName(QStringLiteral("Bottom"))), static_cast<int>(DockEdge::Bottom));
    QCOMPARE(static_cast<int>(dockEdgeFromName(QStringLiteral("unknown"))), static_cast<int>(DockEdge::None));
}

void CaptureBarGeometryTest::clampsBarInsideScreen()
{
    const QRect         screen(0, 0, 1920, 1040);
    const DockPlacement placement = calculateDockPlacement(screen, QSize(320, 48), QPoint(1800, -40), 12, 4, 50);

    QVERIFY(screen.contains(placement.barRect));
    QVERIFY(screen.contains(placement.hotspotRect));
}

void CaptureBarGeometryTest::clampsHotspotInsideRightEdge()
{
    const QRect screen(0, 0, 1920, 1080);
    const QRect bar(1900, 100, 40, 320);
    QCOMPARE(calculateDockHotspot(screen, bar, DockEdge::Top, 4, 50), QRect(1870, 0, 50, 4));
}

QTEST_APPLESS_MAIN(CaptureBarGeometryTest)

#include "capturebargeometrytest.moc"
