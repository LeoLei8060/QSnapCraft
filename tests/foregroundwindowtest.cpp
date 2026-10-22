#include "platform/windows/foregroundwindow.h"

#include <QtTest>

#include <windows.h>
#include <QWidget>

class ForegroundWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void returnsVisibleWindowBounds();
    void rejectsNullWindow();
};

void ForegroundWindowTest::returnsVisibleWindowBounds()
{
    QWidget widget;
    widget.setGeometry(120, 140, 240, 160);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    const QRect bounds = windowFrameBounds(reinterpret_cast<HWND>(widget.winId()));

    QVERIFY(bounds.isValid());
    QVERIFY(bounds.width() >= 240);
    QVERIFY(bounds.height() >= 160);
    QVERIFY(bounds.contains(widget.mapToGlobal(widget.rect().center())));
}

void ForegroundWindowTest::rejectsNullWindow()
{
    QVERIFY(!windowFrameBounds(nullptr).isValid());
}

QTEST_MAIN(ForegroundWindowTest)

#include "foregroundwindowtest.moc"
