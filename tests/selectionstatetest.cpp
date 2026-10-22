#include "capture/selectionstate.h"

#include <QtTest>

class SelectionStateTest : public QObject
{
    Q_OBJECT

private slots:
    void normalizesReverseDrag();
    void resizesFromCenter();
    void movesAndClampsToBounds();
    void replaysHistory();
};

void SelectionStateTest::normalizesReverseDrag()
{
    SelectionState selection(QRect(0, 0, 800, 600));
    selection.beginDrag(QPoint(500, 400));
    selection.dragTo(QPoint(100, 50));

    QCOMPARE(selection.current(), QRect(100, 50, 401, 351));
}

void SelectionStateTest::resizesFromCenter()
{
    SelectionState selection(QRect(0, 0, 800, 600));
    selection.setSelection(QRect(300, 200, 200, 100));
    selection.beginResize(SelectionEdge::kRight, true);
    selection.resizeTo(QPoint(540, 250));

    QCOMPARE(selection.current(), QRect(259, 200, 282, 100));
}

void SelectionStateTest::movesAndClampsToBounds()
{
    SelectionState selection(QRect(0, 0, 800, 600));
    selection.setSelection(QRect(650, 450, 100, 100));
    selection.moveBy(QPoint(100, 100));

    QCOMPARE(selection.current(), QRect(700, 500, 100, 100));
}

void SelectionStateTest::replaysHistory()
{
    SelectionHistory history(2, true);
    history.push(QRect(0, 0, 10, 10));
    history.push(QRect(20, 20, 30, 30));
    history.push(QRect(50, 50, 60, 60));

    QCOMPARE(history.count(), 2);
    QCOMPARE(history.current(), QRect(50, 50, 60, 60));
    QVERIFY(history.previous());
    QCOMPARE(history.current(), QRect(20, 20, 30, 30));
    QVERIFY(history.next());
    QCOMPARE(history.current(), QRect(50, 50, 60, 60));
}

QTEST_MAIN(SelectionStateTest)

#include "selectionstatetest.moc"
