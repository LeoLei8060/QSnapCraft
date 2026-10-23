#include "paint/paintscene.h"
#include "paint/paintitem.h"
#include "paint/painttextitem.h"

#include <QtTest>

#include <QElapsedTimer>
#include <QFont>
#include <QPainter>
#include <QUndoStack>

#include <array>
#include <cstddef>

class PaintSceneTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsToIdleTool();
    void defaultsMosaicBrushToFourteenPixels();
    void finishedItemBecomesEditableSelection();
    void switchingToolFreezesEditedItem();
    void drawingNextItemFreezesPreviousItem();
    void createsAndUndoesRectangle();
    void supportsReversedDrag();
    void deletesSelectedWithUndo();
    void copiesPastesAndSelectsNewItem();
    void nudgesSelectedItemWithUndo();
    void appliesArrowAndEffectProperties();
    void rendersDistinctArrowStyles();
    void rendersHollowArrowWithEmptyCenter();
    void remembersColorPerTool();
    void undoesSelectedPropertyChange();
    void createsAndCopiesText();
    void copiesResizedItemTransform();
    void blursEffectUsingCapturedBackground();
    void blursBrushAcrossEffectTiles();
    void mosaicModesHaveDifferentCoverage();
    void areaMosaicShrinksWithPointer();
    void startsBlurEffectWithoutFullFrameDelay();
    void keepsLargeBlurDragResponsive();
    void areaEraserRestoresBackgroundWithUndo();
    void defersSelectedEffectPropertyRebuild();
};

void PaintSceneTest::defaultsToIdleTool()
{
    PaintScene scene;
    QCOMPARE(scene.tool(), PaintTool::kNone);
}

void PaintSceneTest::defaultsMosaicBrushToFourteenPixels()
{
    PaintScene scene;
    scene.setTool(PaintTool::kMosaic);
    QCOMPARE(scene.penWidth(), 14.0);
}

void PaintSceneTest::finishedItemBecomesEditableSelection()
{
    PaintScene scene;
    scene.setTool(PaintTool::kRectangle);
    PaintItem *item = scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(80, 50));
    scene.finishActiveItem();

    QVERIFY(item->isSelected());
    QVERIFY(item->flags().testFlag(QGraphicsItem::ItemIsMovable));
    QVERIFY(item->flags().testFlag(QGraphicsItem::ItemIsSelectable));
}

void PaintSceneTest::switchingToolFreezesEditedItem()
{
    PaintScene scene;
    scene.setTool(PaintTool::kRectangle);
    PaintItem *item = scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(80, 50));
    scene.finishActiveItem();

    scene.setTool(PaintTool::kPencil);
    QVERIFY(!item->isSelected());
    QVERIFY(!item->flags().testFlag(QGraphicsItem::ItemIsMovable));
    QVERIFY(!item->flags().testFlag(QGraphicsItem::ItemIsSelectable));
}

void PaintSceneTest::drawingNextItemFreezesPreviousItem()
{
    PaintScene scene;
    scene.setTool(PaintTool::kRectangle);
    PaintItem *first = scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(80, 50));
    scene.finishActiveItem();

    PaintItem *second = scene.createItemAt(QPointF(100, 100));
    QVERIFY(!first->isSelected());
    QVERIFY(!first->flags().testFlag(QGraphicsItem::ItemIsMovable));
    scene.updateActiveItem(QPointF(180, 150));
    scene.finishActiveItem();

    QVERIFY(second->isSelected());
    QVERIFY(second->flags().testFlag(QGraphicsItem::ItemIsMovable));
    QVERIFY(!first->isSelected());
}

void PaintSceneTest::createsAndUndoesRectangle()
{
    PaintScene scene;
    scene.setTool(PaintTool::kRectangle);
    PaintItem *item = scene.createItemAt(QPointF(10, 10));
    QVERIFY(item);
    scene.updateActiveItem(QPointF(80, 50));
    scene.finishActiveItem();

    QCOMPARE(scene.visibleItemCount(), 1);
    QVERIFY(scene.undoStack()->canUndo());
    scene.undoStack()->undo();
    QCOMPARE(scene.visibleItemCount(), 0);
    scene.undoStack()->redo();
    QCOMPARE(scene.visibleItemCount(), 1);
}

void PaintSceneTest::supportsReversedDrag()
{
    PaintScene scene;
    scene.setTool(PaintTool::kEllipse);
    PaintItem *item = scene.createItemAt(QPointF(100, 80));
    scene.updateActiveItem(QPointF(20, 10));
    scene.finishActiveItem();

    QCOMPARE(item->mapRectToScene(item->shape().boundingRect()), QRectF(20, 10, 80, 70));
}

void PaintSceneTest::deletesSelectedWithUndo()
{
    PaintScene scene;
    scene.setTool(PaintTool::kRectangle);
    PaintItem *item = scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(80, 50));
    scene.finishActiveItem();

    scene.deleteSelected();
    QCOMPARE(scene.visibleItemCount(), 0);

    scene.undoStack()->undo();
    QCOMPARE(scene.visibleItemCount(), 1);
}

void PaintSceneTest::copiesPastesAndSelectsNewItem()
{
    PaintScene scene;
    scene.setTool(PaintTool::kRectangle);
    scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(80, 50));
    scene.finishActiveItem();

    scene.copySelected();
    scene.pasteCopied();

    QCOMPARE(scene.visibleItemCount(), 2);
    QCOMPARE(scene.selectedItems().size(), 1);
    QVERIFY(scene.hasEditableItem());
    QVERIFY(scene.selectedItems().first() != nullptr);
}

void PaintSceneTest::nudgesSelectedItemWithUndo()
{
    PaintScene scene;
    scene.setTool(PaintTool::kRectangle);
    scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(80, 50));
    scene.finishActiveItem();

    QGraphicsItem *item = scene.editableItem();
    QVERIFY(item);
    const QPointF start = item->pos();
    QKeyEvent     right_event(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    QCoreApplication::sendEvent(&scene, &right_event);
    QCOMPARE(item->pos(), start + QPointF(1, 0));

    QKeyEvent shifted_down(QEvent::KeyPress, Qt::Key_Down, Qt::ShiftModifier);
    QCoreApplication::sendEvent(&scene, &shifted_down);
    QCOMPARE(item->pos(), start + QPointF(1, 10));

    scene.undoStack()->undo();
    QCOMPARE(item->pos(), start + QPointF(1, 0));
}

void PaintSceneTest::appliesArrowAndEffectProperties()
{
    PaintScene scene;
    scene.setArrowStyle(ArrowStyle::kHollowTail);
    scene.setTool(PaintTool::kArrow);
    PaintItem *arrow = scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(80, 40));
    scene.finishActiveItem();

    QCOMPARE(arrow->arrowStyle(), ArrowStyle::kHollowTail);

    scene.setEffectMode(PaintEffectMode::kBlur);
    scene.setEffectLevel(12);
    scene.setTool(PaintTool::kMosaic);
    PaintItem *effect = scene.createItemAt(QPointF(20, 20));
    scene.updateActiveItem(QPointF(90, 70));
    scene.finishActiveItem();

    QCOMPARE(effect->effectMode(), PaintEffectMode::kBlur);
    QCOMPARE(effect->effectLevel(PaintEffectMode::kBlur), 12);
}

void PaintSceneTest::rendersDistinctArrowStyles()
{
    const std::array<ArrowStyle, 3> styles = {
        ArrowStyle::kSolid,
        ArrowStyle::kSolidTail,
        ArrowStyle::kHollowTail,
    };
    std::array<QImage, 3> images;

    for (std::size_t index = 0; index < styles.size(); ++index) {
        PaintScene scene;
        scene.setSceneRect(0, 0, 140, 100);
        scene.setArrowStyle(styles[index]);
        scene.setColor(QColor(255, 255, 255));
        scene.setPenWidth(3);
        scene.setTool(PaintTool::kArrow);
        scene.createItemAt(QPointF(15, 70));
        scene.updateActiveItem(QPointF(120, 25));
        scene.finishActiveItem();

        images[index] = QImage(140, 100, QImage::Format_ARGB32_Premultiplied);
        images[index].fill(Qt::black);
        QPainter painter(&images[index]);
        scene.render(&painter);
    }

    QVERIFY(images[0] != images[1]);
    QVERIFY(images[0] != images[2]);
    QVERIFY(images[1] != images[2]);

    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (!render_directory.isEmpty()) {
        QVERIFY(images[0].save(render_directory + QStringLiteral("/arrow-solid.png")));
        QVERIFY(images[1].save(render_directory + QStringLiteral("/arrow-solid-tail.png")));
        QVERIFY(images[2].save(render_directory + QStringLiteral("/arrow-hollow-tail.png")));
    }
}

void PaintSceneTest::rendersHollowArrowWithEmptyCenter()
{
    PaintScene scene;
    scene.setSceneRect(0, 0, 140, 100);
    scene.setArrowStyle(ArrowStyle::kHollowTail);
    scene.setColor(QColor(235, 28, 36));
    scene.setPenWidth(3);
    scene.setTool(PaintTool::kArrow);
    scene.createItemAt(QPointF(15, 70));
    scene.updateActiveItem(QPointF(120, 25));
    scene.finishActiveItem();

    QImage image(140, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    QPainter painter(&image);
    scene.render(&painter);
    painter.end();

    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (!render_directory.isEmpty()) {
        QVERIFY(image.save(render_directory + QStringLiteral("/arrow-hollow-center.png")));
    }
    QVERIFY(image.pixelColor(58, 51).lightness() < 10);
}

void PaintSceneTest::remembersColorPerTool()
{
    PaintScene   scene;
    const QColor initial = scene.color();
    const QColor green(40, 180, 80);

    scene.setTool(PaintTool::kArrow);
    scene.setColor(green);
    scene.setTool(PaintTool::kPencil);
    QCOMPARE(scene.color(), initial);
    scene.setTool(PaintTool::kArrow);
    QCOMPARE(scene.color(), green);
}

void PaintSceneTest::undoesSelectedPropertyChange()
{
    PaintScene scene;
    scene.setTool(PaintTool::kRectangle);
    PaintItem *item = scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(80, 50));
    scene.finishActiveItem();

    const QColor original = item->color();
    const QColor changed(30, 120, 220);
    scene.setColor(changed);
    QCOMPARE(item->color(), changed);

    scene.undoStack()->undo();
    QCOMPARE(item->color(), original);
}

void PaintSceneTest::createsAndCopiesText()
{
    PaintScene scene;
    scene.setFontSize(36);
    scene.setFontBold(true);
    scene.setFontItalic(true);
    scene.setTextOutlined(true);
    scene.setTextBackgroundEnabled(true);

    PaintTextItem *text = scene.createTextAt(QPointF(15, 20));
    text->setPlainText(QStringLiteral("QSnapCraft"));

    QCOMPARE(text->font().pixelSize(), 36);
    QVERIFY(text->font().bold());
    QVERIFY(text->font().italic());
    QVERIFY(text->isOutlined());
    QVERIFY(text->hasBackground());

    scene.copySelected();
    scene.pasteCopied();
    QCOMPARE(scene.visibleItemCount(), 2);
}

void PaintSceneTest::copiesResizedItemTransform()
{
    PaintScene scene;
    scene.setTool(PaintTool::kRectangle);
    PaintItem *item = scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(110, 70));
    scene.finishActiveItem();

    const QTransform transform = QTransform::fromScale(1.75, 0.6);
    item->setTransform(transform);
    scene.copySelected();
    scene.pasteCopied();

    PaintItem *copy = nullptr;
    for (QGraphicsItem *scene_item : scene.items()) {
        auto *candidate = dynamic_cast<PaintItem *>(scene_item);
        if (candidate != nullptr && candidate != item) {
            copy = candidate;
            break;
        }
    }
    QVERIFY(copy);
    QCOMPARE(copy->transform(), transform);
}

void PaintSceneTest::blursEffectUsingCapturedBackground()
{
    QImage background(320, 240, QImage::Format_ARGB32);
    for (int y = 0; y < background.height(); ++y) {
        for (int x = 0; x < background.width(); ++x) {
            background.setPixelColor(x, y, (x + y) % 2 == 0 ? Qt::white : Qt::black);
        }
    }

    PaintScene scene;
    scene.setSceneRect(0, 0, 320, 240);
    scene.setBackgroundImage(background);
    scene.setEffectMode(PaintEffectMode::kBlur);
    scene.setEffectLevel(6);
    scene.setFilled(true);
    scene.setTool(PaintTool::kMosaic);
    scene.createItemAt(QPointF(4, 4));
    scene.updateActiveItem(QPointF(316, 236));
    scene.finishActiveItem();

    QImage rendered(320, 240, QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    QPainter painter(&rendered);
    scene.render(&painter);
    painter.end();

    for (const QPoint &point : {QPoint(40, 40), QPoint(140, 120), QPoint(280, 200)}) {
        const QColor actual = rendered.pixelColor(point);
        const QColor expected = background.pixelColor(point);
        QCOMPARE(actual.alpha(), 255);
        QVERIFY2(actual != expected,
                 qPrintable(QStringLiteral("point=%1,%2 actual=%3 expected=%4")
                                .arg(point.x())
                                .arg(point.y())
                                .arg(actual.name(), expected.name())));
    }
}

void PaintSceneTest::blursBrushAcrossEffectTiles()
{
    QImage background(320, 240, QImage::Format_ARGB32);
    for (int y = 0; y < background.height(); ++y) {
        for (int x = 0; x < background.width(); ++x) {
            background.setPixelColor(x, y, (x + y) % 2 == 0 ? Qt::white : Qt::black);
        }
    }

    PaintScene scene;
    scene.setSceneRect(0, 0, 320, 240);
    scene.setBackgroundImage(background);
    scene.setEffectMode(PaintEffectMode::kBlur);
    scene.setEffectLevel(6);
    scene.setFilled(false);
    scene.setTool(PaintTool::kMosaic);
    scene.setPenWidth(12);
    scene.createItemAt(QPointF(20, 100));
    scene.updateActiveItem(QPointF(300, 100));
    scene.finishActiveItem();

    QImage rendered(320, 240, QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    QPainter painter(&rendered);
    scene.render(&painter);
    painter.end();

    for (const QPoint &point : {QPoint(60, 100), QPoint(170, 100), QPoint(260, 100)}) {
        const QColor actual = rendered.pixelColor(point);
        const QColor expected = background.pixelColor(point);
        QCOMPARE(actual.alpha(), 255);
        QVERIFY2(actual != expected,
                 qPrintable(QStringLiteral("point=%1,%2 actual=%3 expected=%4")
                                .arg(point.x())
                                .arg(point.y())
                                .arg(actual.name(), expected.name())));
    }
}

void PaintSceneTest::mosaicModesHaveDifferentCoverage()
{
    QImage background(300, 300, QImage::Format_ARGB32);
    for (int y = 0; y < background.height(); ++y) {
        for (int x = 0; x < background.width(); ++x) {
            background.setPixelColor(x, y, (x + y) % 2 == 0 ? Qt::white : Qt::black);
        }
    }

    const auto render_mosaic = [&background](bool filled) {
        PaintScene scene;
        scene.setSceneRect(0, 0, 300, 300);
        scene.setBackgroundImage(background);
        scene.setEffectMode(PaintEffectMode::kMosaic);
        scene.setEffectLevel(8);
        scene.setFilled(filled);
        scene.setTool(PaintTool::kMosaic);
        scene.createItemAt(QPointF(20, 20));
        scene.updateActiveItem(filled ? QPointF(280, 280) : QPointF(280, 20));
        scene.finishActiveItem();

        QImage rendered(300, 300, QImage::Format_ARGB32_Premultiplied);
        rendered.fill(Qt::transparent);
        QPainter painter(&rendered);
        scene.render(&painter);
        return rendered;
    };

    const QImage area = render_mosaic(true);
    const QImage brush = render_mosaic(false);
    QVERIFY(area.pixelColor(30, 25).alpha() > 0);
    QVERIFY(area.pixelColor(150, 150).alpha() > 0);
    QVERIFY(area.pixelColor(250, 250).alpha() > 0);
    QCOMPARE(brush.pixelColor(30, 50).alpha(), 0);
    QVERIFY(brush.pixelColor(150, 20).alpha() > 0);
}

void PaintSceneTest::areaMosaicShrinksWithPointer()
{
    QImage background(300, 300, QImage::Format_ARGB32);
    background.fill(Qt::white);

    PaintScene scene;
    scene.setSceneRect(0, 0, 300, 300);
    scene.setBackgroundImage(background);
    scene.setEffectMode(PaintEffectMode::kMosaic);
    scene.setEffectLevel(8);
    scene.setFilled(true);
    scene.setTool(PaintTool::kMosaic);
    scene.createItemAt(QPointF(20, 20));
    scene.updateActiveItem(QPointF(280, 280));
    scene.updateActiveItem(QPointF(80, 80));
    scene.finishActiveItem();

    QImage rendered(300, 300, QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    QPainter painter(&rendered);
    scene.render(&painter);
    painter.end();

    QVERIFY(rendered.pixelColor(50, 50).alpha() > 0);
    QCOMPARE(rendered.pixelColor(200, 200).alpha(), 0);
}

void PaintSceneTest::startsBlurEffectWithoutFullFrameDelay()
{
    QImage background(5120, 1440, QImage::Format_ARGB32);
    background.fill(QColor(40, 80, 120));

    PaintScene scene;
    scene.setSceneRect(QRect(QPoint(), background.size()));
    scene.setBackgroundImage(background);
    scene.setEffectMode(PaintEffectMode::kBlur);
    scene.setEffectLevel(8);
    scene.setFilled(true);
    scene.setTool(PaintTool::kMosaic);

    QElapsedTimer timer;
    timer.start();
    PaintItem *item = scene.createItemAt(QPointF(2000, 700));
    scene.updateActiveItem(QPointF(2400, 900));
    const qint64 elapsed_ms = timer.elapsed();

    QVERIFY(item);
    QVERIFY2(elapsed_ms < 400, qPrintable(QStringLiteral("elapsed_ms=%1").arg(elapsed_ms)));
    scene.finishActiveItem();
}

void PaintSceneTest::keepsLargeBlurDragResponsive()
{
    QImage background(5120, 1440, QImage::Format_ARGB32);
    background.fill(QColor(40, 80, 120));

    PaintScene scene;
    scene.setSceneRect(QRect(QPoint(), background.size()));
    scene.setBackgroundImage(background);
    scene.setEffectMode(PaintEffectMode::kBlur);
    scene.setEffectLevel(8);
    scene.setFilled(true);
    scene.setTool(PaintTool::kMosaic);
    scene.createItemAt(QPointF(2, 2));

    QElapsedTimer timer;
    timer.start();
    scene.updateActiveItem(QPointF(5118, 1438));
    const qint64 elapsed_ms = timer.elapsed();

    QVERIFY2(elapsed_ms < 200, qPrintable(QStringLiteral("elapsed_ms=%1").arg(elapsed_ms)));
}

void PaintSceneTest::areaEraserRestoresBackgroundWithUndo()
{
    PaintScene scene;
    scene.setSceneRect(QRectF(0, 0, 100, 100));
    QImage background(100, 100, QImage::Format_ARGB32_Premultiplied);
    background.fill(Qt::red);
    scene.setBackgroundImage(background);

    scene.setTool(PaintTool::kRectangle);
    scene.setColor(Qt::blue);
    scene.setFilled(true);
    scene.createItemAt(QPointF(10, 10));
    scene.updateActiveItem(QPointF(90, 70));
    scene.finishActiveItem();
    scene.stopItemEditing();

    scene.setTool(PaintTool::kEraser);
    scene.setFilled(true);
    scene.createItemAt(QPointF(20, 20));
    scene.updateActiveItem(QPointF(60, 50));
    scene.finishActiveItem();
    scene.stopItemEditing();

    QImage rendered(100, 100, QImage::Format_ARGB32_Premultiplied);
    rendered.fill(Qt::transparent);
    QPainter painter(&rendered);
    scene.render(&painter, QRectF(rendered.rect()), QRectF(rendered.rect()));
    painter.end();
    QCOMPARE(rendered.pixelColor(40, 35), QColor(Qt::red));
    QCOMPARE(rendered.pixelColor(80, 70), QColor(Qt::blue));

    scene.undoStack()->undo();
    QImage undone(100, 100, QImage::Format_ARGB32_Premultiplied);
    undone.fill(Qt::transparent);
    painter.begin(&undone);
    scene.render(&painter, QRectF(undone.rect()), QRectF(undone.rect()));
    painter.end();
    QCOMPARE(undone.pixelColor(40, 35), QColor(Qt::blue));
}

void PaintSceneTest::defersSelectedEffectPropertyRebuild()
{
    QImage background(1024, 1024, QImage::Format_ARGB32);
    background.fill(QColor(40, 80, 120));

    PaintScene scene;
    scene.setSceneRect(QRect(QPoint(), background.size()));
    scene.setBackgroundImage(background);
    scene.setEffectMode(PaintEffectMode::kBlur);
    scene.setEffectLevel(8);
    scene.setFilled(true);
    scene.setTool(PaintTool::kMosaic);
    PaintItem *item = scene.createItemAt(QPointF(4, 4));
    scene.updateActiveItem(QPointF(1020, 1020));
    scene.finishActiveItem();

    QElapsedTimer timer;
    timer.start();
    scene.setEffectLevel(24);
    const qint64 elapsed_ms = timer.elapsed();

    QVERIFY2(elapsed_ms < 100, qPrintable(QStringLiteral("elapsed_ms=%1").arg(elapsed_ms)));
    QVERIFY(item->hasPendingEffectTiles());
    QTRY_VERIFY_WITH_TIMEOUT(!item->hasPendingEffectTiles(), 1000);
}

QTEST_MAIN(PaintSceneTest)

#include "paintscenetest.moc"
