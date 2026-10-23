#include "pinboard/pinscreenfit.h"

#include <QtTest>

class PinScreenFitTest : public QObject
{
    Q_OBJECT

private slots:
    void keepsRectCoveredByAdjacentScreens();
    void movesRectOutOfIrregularScreenGap();
};

void PinScreenFitTest::keepsRectCoveredByAdjacentScreens()
{
    const QRect          frame(700, 100, 200, 120);
    const QVector<QRect> screens = {
        QRect(0, 0, 800, 600),
        QRect(800, 0, 800, 600),
    };

    QCOMPARE(clampPinToScreens(frame, screens), frame.topLeft());
}

void PinScreenFitTest::movesRectOutOfIrregularScreenGap()
{
    const QRect          frame(100, 700, 100, 100);
    const QVector<QRect> screens = {
        QRect(0, 0, 800, 600),
        QRect(800, 600, 800, 600),
    };

    const QPoint position = clampPinToScreens(frame, screens);
    QVERIFY(screens.constFirst().contains(QRect(position, frame.size())));
}

QTEST_APPLESS_MAIN(PinScreenFitTest)

#include "pinscreenfittest.moc"
