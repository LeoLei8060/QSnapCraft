#include "ui/screenrulerwindow.h"

#include <QtTest>

#include <QApplication>
#include <QKeyEvent>
#include <QToolButton>

class ScreenRulerWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void showsVerticalRulerAtCursor();
    void switchesOrientationWithoutRemovingTicks();
    void exposesIconOnlyControls();
    void opensAndCancelsFullscreenRectangleMeasurement();
    void completesFullscreenRectangleMeasurement();
};

void ScreenRulerWindowTest::showsVerticalRulerAtCursor()
{
    ScreenRulerWindow ruler;
    ruler.showAtCursor();
    QVERIFY(ruler.isVisible());
    QCOMPARE(ruler.orientation(), RulerOrientation::kVertical);
    QCOMPARE(ruler.width(), 100);
    QVERIFY(ruler.height() >= 200);
    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (!render_directory.isEmpty()) {
        QVERIFY(ruler.grab().save(render_directory + QStringLiteral("/screen-ruler-vertical.png")));
    }
    ruler.close();
}

void ScreenRulerWindowTest::switchesOrientationWithoutRemovingTicks()
{
    ScreenRulerWindow ruler;
    ruler.showAtCursor();
    ruler.setOrientation(RulerOrientation::kHorizontal);
    QCOMPARE(ruler.orientation(), RulerOrientation::kHorizontal);
    QCOMPARE(ruler.height(), 100);
    QVERIFY(ruler.width() >= 250);
    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (!render_directory.isEmpty()) {
        QVERIFY(ruler.grab().save(render_directory + QStringLiteral("/screen-ruler-horizontal.png")));
    }
    ruler.setOrientation(RulerOrientation::kVertical);
    QCOMPARE(ruler.orientation(), RulerOrientation::kVertical);
    QCOMPARE(ruler.width(), 100);
    ruler.close();
}

void ScreenRulerWindowTest::exposesIconOnlyControls()
{
    ScreenRulerWindow          ruler;
    const QList<QToolButton *> buttons = ruler.findChildren<QToolButton *>();
    QCOMPARE(buttons.size(), 6);
    for (QToolButton *button : buttons) {
        QVERIFY(!button->objectName().isEmpty());
        QVERIFY(!button->icon().isNull());
        QVERIFY(button->text().isEmpty());
        QVERIFY(!button->toolTip().isEmpty());
    }
}

void ScreenRulerWindowTest::opensAndCancelsFullscreenRectangleMeasurement()
{
    ScreenRulerWindow ruler;
    ruler.showAtCursor();
    QToolButton *rectangle = ruler.findChild<QToolButton *>(QStringLiteral("rulerRectangle"));
    QVERIFY(rectangle);
    QTest::mouseClick(rectangle, Qt::LeftButton);

    QWidget *overlay = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT(([&overlay]() {
                                 for (QWidget *widget : QApplication::topLevelWidgets()) {
                                     if (widget->objectName() == QStringLiteral("rulerMeasurementOverlay")) {
                                         overlay = widget;
                                         return true;
                                     }
                                 }
                                 return false;
                             })(),
                             3000);
    QVERIFY(overlay);
    QVERIFY(overlay->isVisible());
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QCoreApplication::sendEvent(overlay, &escape);
    QTRY_VERIFY_WITH_TIMEOUT(ruler.isVisible(), 1000);
    ruler.close();
}

void ScreenRulerWindowTest::completesFullscreenRectangleMeasurement()
{
    ScreenRulerWindow ruler;
    ruler.showAtCursor();
    QToolButton *rectangle = ruler.findChild<QToolButton *>(QStringLiteral("rulerRectangle"));
    QVERIFY(rectangle);
    QTest::mouseClick(rectangle, Qt::LeftButton);

    QWidget *overlay = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT(([&overlay]() {
                                 for (QWidget *widget : QApplication::topLevelWidgets()) {
                                     if (widget->objectName() == QStringLiteral("rulerMeasurementOverlay")) {
                                         overlay = widget;
                                         return true;
                                     }
                                 }
                                 return false;
                             })(),
                             3000);
    QVERIFY(overlay);

    QTest::mousePress(overlay, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseRelease(overlay, Qt::LeftButton, Qt::NoModifier, QPoint(300, 220));
    QTRY_VERIFY_WITH_TIMEOUT(ruler.isVisible(), 1000);
    QVERIFY(ruler.lastMeasurement().contains(QStringLiteral(" x ")));
    ruler.close();
}

QTEST_MAIN(ScreenRulerWindowTest)

#include "screenrulerwindowtest.moc"
