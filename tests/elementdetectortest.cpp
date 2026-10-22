#include "platform/windows/elementdetector.h"

#include <QtTest>

class ElementDetectorTest : public QObject
{
    Q_OBJECT

private slots:
    void rejectsInvalidCandidate();
};

void ElementDetectorTest::rejectsInvalidCandidate()
{
    ElementCandidate candidate;
    QVERIFY(!candidate.isValid());

    candidate.rect = QRect(10, 20, 30, 40);
    QVERIFY(candidate.isValid());
}

QTEST_MAIN(ElementDetectorTest)

#include "elementdetectortest.moc"
