#include "pinboard/textimagerenderer.h"

#include <QtTest>

class TextImageRendererTest : public QObject
{
    Q_OBJECT

private slots:
    void rendersPlainTextWithinConfiguredWidth();
    void rendersHtmlContent();
    void rejectsEmptyContent();
};

void TextImageRendererTest::rendersPlainTextWithinConfiguredWidth()
{
    TextToImageOptions options;
    options.maxWidth = 240;
    options.padding = 12;
    options.fontSize = 18;
    options.backgroundColor = QColor(QStringLiteral("#203040"));
    options.textColor = QColor(Qt::white);

    const QImage image
        = renderTextToImage(QStringLiteral(
                                "QSnapCraft\nA long line that should wrap inside the configured maximum width."),
                            TextImageFormat::kPlainText,
                            options);

    QVERIFY(!image.isNull());
    QVERIFY(image.width() <= options.maxWidth);
    QVERIFY(image.height() > options.padding * 2);
    QCOMPARE(image.pixelColor(image.width() - 1, image.height() - 1), options.backgroundColor);
}

void TextImageRendererTest::rendersHtmlContent()
{
    TextToImageOptions options;
    options.maxWidth = 300;
    options.backgroundColor = QColor(Qt::white);

    const QImage image
        = renderTextToImage(QStringLiteral("<p><b>QSnapCraft</b><br/><span style=\"color:#d94e3e\">HTML</span></p>"),
                            TextImageFormat::kHtml,
                            options);

    QVERIFY(!image.isNull());
    QVERIFY(image.width() <= options.maxWidth);
    QVERIFY(image.width() > options.padding * 2);
    QVERIFY(image.height() > options.padding * 2);
}

void TextImageRendererTest::rejectsEmptyContent()
{
    QVERIFY(renderTextToImage({}, TextImageFormat::kPlainText, TextToImageOptions{}).isNull());
    QVERIFY(renderTextToImage(QStringLiteral("   \n"), TextImageFormat::kPlainText, TextToImageOptions{}).isNull());
}

QTEST_MAIN(TextImageRendererTest)

#include "textimagerenderertest.moc"
