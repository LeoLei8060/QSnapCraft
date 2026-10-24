#include "capture/scrollstitcher.h"

#include <QPainter>
#include <QtTest>

namespace {
QImage scrollableContent()
{
    QImage image(120, 500, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    for (int y = 0; y < image.height(); y += 10) {
        painter.fillRect(0, y, image.width(), 4, QColor(y % 200, (y * 3) % 255, (y * 7) % 255));
        painter.drawText(8, y + 8, QString::number(y));
    }
    painter.end();
    return image;
}

QImage horizontallyScrollableContent()
{
    QImage image(500, 120, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    for (int x = 0; x < image.width(); x += 10) {
        painter.fillRect(x, 0, 4, image.height(), QColor(x % 200, (x * 3) % 255, (x * 7) % 255));
        painter.drawText(x + 2, 30, QString::number(x));
    }
    painter.end();
    return image;
}
} // namespace

class ScrollStitcherTest : public QObject
{
    Q_OBJECT

private slots:
    void stitchesOverlappingVerticalFrames();
    void stitchesOverlappingHorizontalFrames();
    void detectsScrollViewportInsideFixedMargins();
    void detectsHorizontalScrollViewportInsideFixedMargins();
    void rejectsStaticFrame();
};

void ScrollStitcherTest::stitchesOverlappingVerticalFrames()
{
    const QImage content = scrollableContent();
    const int    frame_height = 160;
    const QImage first = content.copy(0, 0, content.width(), frame_height);
    const QImage second = content.copy(0, 60, content.width(), frame_height);
    const QImage third = content.copy(0, 120, content.width(), frame_height);

    ScrollStitcher stitcher;
    stitcher.reset(first);
    int added = 0;
    QVERIFY(stitcher.appendFrame(second, &added));
    QCOMPARE(added, 60);
    QVERIFY(stitcher.appendFrame(third, &added));
    QCOMPARE(added, 60);

    const QImage expected = content.copy(0, 0, content.width(), frame_height + 120);
    QCOMPARE(stitcher.image(), expected);
}

void ScrollStitcherTest::stitchesOverlappingHorizontalFrames()
{
    const QImage content = horizontallyScrollableContent();
    const int    frame_width = 160;
    const QImage first = content.copy(0, 0, frame_width, content.height());
    const QImage second = content.copy(60, 0, frame_width, content.height());
    const QImage third = content.copy(120, 0, frame_width, content.height());

    ScrollStitcher stitcher;
    stitcher.reset(first);
    int added = 0;
    QVERIFY(stitcher.appendFrame(second, &added));
    QCOMPARE(added, 60);
    QVERIFY(stitcher.appendFrame(third, &added));
    QCOMPARE(added, 60);

    const QImage expected = content.copy(0, 0, frame_width + 120, content.height());
    QCOMPARE(stitcher.image(), expected);
}

void ScrollStitcherTest::detectsScrollViewportInsideFixedMargins()
{
    QImage content(200, 500, QImage::Format_ARGB32_Premultiplied);
    content.fill(Qt::white);
    QPainter painter(&content);
    for (int y = 0; y < content.height(); y += 8) {
        painter.fillRect(0, y, content.width(), 3, QColor(y % 255, (y * 5) % 255, (y * 11) % 255));
    }
    painter.end();

    const auto frame_at = [&content](int offset) {
        QImage frame(260, 190, QImage::Format_ARGB32_Premultiplied);
        frame.fill(QColor(QStringLiteral("#505050")));
        QPainter painter(&frame);
        painter.fillRect(0, 0, frame.width(), 40, QColor(QStringLiteral("#205080")));
        painter.fillRect(0, 160, frame.width(), 30, QColor(QStringLiteral("#308050")));
        painter.fillRect(0, 40, 20, 120, QColor(QStringLiteral("#805020")));
        painter.drawImage(QPoint(20, 40), content, QRect(0, offset, 200, 120));
        return frame;
    };

    ScrollStitcher stitcher;
    stitcher.reset(frame_at(0));
    QVERIFY(stitcher.appendFrame(frame_at(60)));
    QVERIFY(stitcher.appendFrame(frame_at(120)));

    QCOMPARE(stitcher.viewport(), QRect(20, 40, 200, 120));
    QCOMPARE(stitcher.image(), content.copy(0, 0, 200, 240));
}

void ScrollStitcherTest::detectsHorizontalScrollViewportInsideFixedMargins()
{
    const QImage content = horizontallyScrollableContent();
    const auto   frameAt = [&content](int offset) {
        QImage frame(220, 180, QImage::Format_ARGB32_Premultiplied);
        frame.fill(QColor(QStringLiteral("#505050")));
        QPainter painter(&frame);
        painter.fillRect(0, 0, 20, frame.height(), QColor(QStringLiteral("#205080")));
        painter.fillRect(200, 0, 20, frame.height(), QColor(QStringLiteral("#308050")));
        painter.fillRect(20, 0, 180, 30, QColor(QStringLiteral("#805020")));
        painter.fillRect(20, 150, 180, 30, QColor(QStringLiteral("#805020")));
        painter.drawImage(QPoint(40, 30), content, QRect(offset, 0, 140, 120));
        return frame;
    };

    ScrollStitcher stitcher;
    stitcher.reset(frameAt(0));
    QVERIFY(stitcher.appendFrame(frameAt(60)));
    QVERIFY(stitcher.appendFrame(frameAt(120)));

    QCOMPARE(stitcher.viewport(), QRect(40, 30, 140, 120));
    QCOMPARE(stitcher.image(), content.copy(0, 0, 260, 120));
}

void ScrollStitcherTest::rejectsStaticFrame()
{
    const QImage   frame = scrollableContent().copy(0, 0, 120, 160);
    ScrollStitcher stitcher;
    stitcher.reset(frame);
    QVERIFY(!stitcher.appendFrame(frame));
    QCOMPARE(stitcher.image(), frame);
}

QTEST_MAIN(ScrollStitcherTest)

#include "scrollstitchertest.moc"
