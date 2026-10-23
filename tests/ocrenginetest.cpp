#include "platform/windows/ocrengine.h"
#include "ui/ocrdialog.h"

#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QtTest>

class OcrEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void recognizesRenderedText();
    void ocrDialogCopiesEditedText();
};

void OcrEngineTest::recognizesRenderedText()
{
    if (!isOcrEngineAvailable()) {
        QSKIP("Windows OCR language engine is unavailable");
    }

    QImage image(720, 180, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setPen(Qt::black);
    QFont font(QStringLiteral("Arial"));
    font.setPixelSize(72);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(image.rect(), Qt::AlignCenter, QStringLiteral("Hello 2025"));
    painter.end();

    const OcrRecognitionResult result = recognizeImageText(image);
    QVERIFY2(result.success, qPrintable(result.error));
    QVERIFY(!result.text.trimmed().isEmpty());
}

void OcrEngineTest::ocrDialogCopiesEditedText()
{
    OcrDialog dialog(QStringLiteral("Original text"));
    auto     *editor = dialog.findChild<QPlainTextEdit *>(QStringLiteral("edit_OcrText"));
    auto     *copy = dialog.findChild<QPushButton *>(QStringLiteral("btn_CopyOcrText"));
    QVERIFY(editor);
    QVERIFY(copy);

    editor->setPlainText(QStringLiteral("Edited text"));
    QApplication::clipboard()->clear();
    QTest::mouseClick(copy, Qt::LeftButton);
    QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("Edited text"));
}

QTEST_MAIN(OcrEngineTest)

#include "ocrenginetest.moc"
