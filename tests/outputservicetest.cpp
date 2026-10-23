#include "output/outputservice.h"

#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

class OutputServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void adjustsExtensionToSelectedFilter();
    void keepsSupportedExtension();
    void reportsInvalidDirectory();
};

void OutputServiceTest::adjustsExtensionToSelectedFilter()
{
    QCOMPARE(OutputService::ensurePathExtension(QStringLiteral("capture.PNG"),
                                                QStringLiteral("JPEG 图像 (*.jpg *.jpeg)")),
             QStringLiteral("capture.jpg"));
    QCOMPARE(OutputService::ensurePathExtension(QStringLiteral("capture"), QStringLiteral("WebP 图像 (*.webp)")),
             QStringLiteral("capture.webp"));
}

void OutputServiceTest::keepsSupportedExtension()
{
    QCOMPARE(OutputService::ensurePathExtension(QStringLiteral("capture.jpeg"),
                                                QStringLiteral("JPEG 图像 (*.jpg *.jpeg)")),
             QStringLiteral("capture.jpeg"));
    QCOMPARE(OutputService::ensurePathExtension(QStringLiteral("capture.custom"), {}), QStringLiteral("capture.custom"));
}

void OutputServiceTest::reportsInvalidDirectory()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString blocker_path = directory.filePath(QStringLiteral("blocked"));
    QFile         blocker(blocker_path);
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.write("not a directory");
    blocker.close();

    QImage image(2, 2, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    QString error;
    QVERIFY(!OutputService::saveImage(image,
                                      blocker_path + QStringLiteral("/capture.png"),
                                      QStringLiteral("PNG"),
                                      95,
                                      &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(OutputServiceTest)

#include "outputservicetest.moc"
