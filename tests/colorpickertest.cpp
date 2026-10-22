#include "ui/colorpickerwindow.h"

#include <QtTest>

class ColorPickerTest : public QObject
{
    Q_OBJECT

private slots:
    void readsPixelColor();
    void rejectsOutOfBoundsPixel();
};

void ColorPickerTest::readsPixelColor()
{
    QImage image(8, 8, QImage::Format_ARGB32);
    image.fill(QColor(12, 34, 56));

    QCOMPARE(ColorPickerWindow::colorAt(image, QPoint(3, 4)), QColor(12, 34, 56));
}

void ColorPickerTest::rejectsOutOfBoundsPixel()
{
    QImage image(8, 8, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QVERIFY(!ColorPickerWindow::colorAt(image, QPoint(8, 4)).isValid());
}

QTEST_MAIN(ColorPickerTest)

#include "colorpickertest.moc"
