#include "paint/paintscene.h"
#include "ui/paintbar.h"

#include <QComboBox>
#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QSlider>
#include <QToolButton>
#include <QWheelEvent>
#include <QtTest>

class PaintBarTest : public QObject
{
    Q_OBJECT

private slots:
    void omitsSelectTool();
    void keepsCommonPropertiesVisibleForRectangle();
    void ordersCommonProperties();
    void arrowStylesUseExclusiveButtons();
    void showsTwentyColorPaletteAtEnd();
    void keepsColorAfterTextProperties();
    void mosaicModesUseButtonsAndSlider();
    void mosaicBrushSliderControlsPenWidth();
    void externalPenWidthUpdatesMosaicSlider();
    void wheelChangesSize();
    void rendersToolbarLayouts();
};

void PaintBarTest::omitsSelectTool()
{
    PaintBar bar;
    QVERIFY(!bar.findChild<QToolButton *>(QStringLiteral("btn_ToolSelect")));

    int tool_count = 0;
    for (QToolButton *button : bar.findChildren<QToolButton *>()) {
        if (button->property("paintTool").isValid()) {
            ++tool_count;
            QVERIFY(button->property("paintTool").toInt() != static_cast<int>(PaintTool::kSelect));
        }
    }
    QCOMPARE(tool_count, 10);
}

void PaintBarTest::keepsCommonPropertiesVisibleForRectangle()
{
    PaintBar bar;
    bar.setCurrentTool(PaintTool::kRectangle);
    bar.show();
    QCoreApplication::processEvents();

    auto *row = bar.findChild<QWidget *>(QStringLiteral("row_PaintProperties"));
    auto *size = bar.findChild<QToolButton *>(QStringLiteral("btn_PaintSize"));
    auto *fill = bar.findChild<QToolButton *>(QStringLiteral("btn_StrokeFill"));
    auto *color = bar.findChild<QToolButton *>(QStringLiteral("btn_PaintColor"));
    auto *opacity = bar.findChild<QWidget *>(QStringLiteral("spin_MarkerOpacity"));

    QVERIFY(row);
    QVERIFY(size);
    QVERIFY(fill);
    QVERIFY(color);
    QVERIFY(opacity);
    QVERIFY(row->isVisibleTo(&bar));
    QVERIFY(size->isVisibleTo(&bar));
    QVERIFY(fill->isVisibleTo(&bar));
    QVERIFY(color->isVisibleTo(&bar));
    QVERIFY(!opacity->isVisibleTo(&bar));

    const int size_x = size->mapTo(&bar, QPoint()).x();
    const int fill_x = fill->mapTo(&bar, QPoint()).x();
    const int color_x = color->mapTo(&bar, QPoint()).x();
    QVERIFY(size_x < fill_x);
    QVERIFY(fill_x < color_x);
}

void PaintBarTest::ordersCommonProperties()
{
    PaintBar bar;
    bar.setCurrentTool(PaintTool::kMarker);
    bar.show();
    QCoreApplication::processEvents();

    auto *size = bar.findChild<QToolButton *>(QStringLiteral("btn_PaintSize"));
    auto *opacity = bar.findChild<QWidget *>(QStringLiteral("spin_MarkerOpacity"));
    auto *colorGroup = bar.findChild<QWidget *>(QStringLiteral("container_ColorGroup"));
    QVERIFY(size);
    QVERIFY(opacity);
    QVERIFY(colorGroup);

    const int size_x = size->mapTo(&bar, QPoint()).x();
    const int opacity_x = opacity->mapTo(&bar, QPoint()).x();
    const int color_group_x = colorGroup->mapTo(&bar, QPoint()).x();
    QVERIFY(size_x < opacity_x);
    QVERIFY(opacity_x < color_group_x);
}

void PaintBarTest::arrowStylesUseExclusiveButtons()
{
    PaintBar bar;
    bar.setCurrentTool(PaintTool::kArrow);
    bar.show();
    QCoreApplication::processEvents();

    auto *solid = bar.findChild<QToolButton *>(QStringLiteral("btn_ArrowSolid"));
    auto *solidTail = bar.findChild<QToolButton *>(QStringLiteral("btn_ArrowSolidTail"));
    auto *hollowTail = bar.findChild<QToolButton *>(QStringLiteral("btn_ArrowHollowTail"));
    auto *colorGroup = bar.findChild<QWidget *>(QStringLiteral("container_ColorGroup"));
    QVERIFY(solid);
    QVERIFY(solidTail);
    QVERIFY(hollowTail);
    QVERIFY(colorGroup);
    QVERIFY(!bar.findChild<QComboBox *>(QStringLiteral("combo_ArrowStyle")));
    QCOMPARE(solid->toolTip(), QStringLiteral("实心箭头"));
    QCOMPARE(solidTail->toolTip(), QStringLiteral("实心尾线箭头"));
    QCOMPARE(hollowTail->toolTip(), QStringLiteral("空心尾线箭头"));
    QVERIFY(solid->isChecked());
    QVERIFY(!solidTail->isChecked());
    QVERIFY(!hollowTail->isChecked());
    QVERIFY(hollowTail->mapTo(&bar, QPoint()).x() < colorGroup->mapTo(&bar, QPoint()).x());

    QSignalSpy style_spy(&bar, &PaintBar::sigArrowStyleChanged);
    QTest::mouseClick(solidTail, Qt::LeftButton);

    QCOMPARE(style_spy.count(), 1);
    QCOMPARE(style_spy.at(0).at(0).toInt(), 1);
    QVERIFY(!solid->isChecked());
    QVERIFY(solidTail->isChecked());
    QVERIFY(!hollowTail->isChecked());
}

void PaintBarTest::showsTwentyColorPaletteAtEnd()
{
    PaintBar bar;
    bar.setCurrentTool(PaintTool::kRectangle);
    bar.show();
    QCoreApplication::processEvents();

    auto *colorGroup = bar.findChild<QWidget *>(QStringLiteral("container_ColorGroup"));
    auto *palette = colorGroup
                        ? colorGroup->findChild<QWidget *>(QStringLiteral("container_ColorPalette"))
                        : nullptr;
    auto *color = colorGroup ? colorGroup->findChild<QToolButton *>(QStringLiteral("btn_PaintColor")) : nullptr;
    QVERIFY(colorGroup);
    QVERIFY(palette);
    QVERIFY(color);
    QVERIFY(color->mapTo(colorGroup, QPoint()).x() < palette->mapTo(colorGroup, QPoint()).x());

    QVector<QToolButton *> swatches;
    for (QToolButton *button : palette->findChildren<QToolButton *>()) {
        if (button->property("paletteColor").isValid()) {
            swatches.append(button);
        }
    }
    QCOMPARE(swatches.size(), 20);
    QCOMPARE(swatches.at(0)->property("paletteColor").value<QColor>(), QColor(QStringLiteral("#000000")));
    QCOMPARE(swatches.at(10)->property("paletteColor").value<QColor>(), QColor(QStringLiteral("#FFFFFF")));
    QCOMPARE(swatches.at(19)->property("paletteColor").value<QColor>(), QColor(QStringLiteral("#C8BFE7")));

    const int first_row_y = swatches.at(0)->mapTo(palette, QPoint()).y();
    const int second_row_y = swatches.at(10)->mapTo(palette, QPoint()).y();
    const int color_top = color->mapTo(colorGroup, QPoint()).y();
    const int color_bottom = color_top + color->height() - 1;
    const int swatch_top = swatches.at(0)->mapTo(colorGroup, QPoint()).y();
    const int swatch_bottom = swatches.at(19)->mapTo(colorGroup, QPoint()).y() + swatches.at(19)->height() - 1;
    QCOMPARE(color_top, swatch_top);
    QCOMPARE(color_bottom, swatch_bottom);
    QVERIFY(second_row_y > first_row_y);
    for (int index = 1; index < 10; ++index) {
        QCOMPARE(swatches.at(index)->mapTo(palette, QPoint()).y(), first_row_y);
        QCOMPARE(swatches.at(index + 10)->mapTo(palette, QPoint()).y(), second_row_y);
    }

    QSignalSpy color_spy(&bar, &PaintBar::sigColorChanged);
    QTest::mouseClick(swatches.at(9), Qt::LeftButton);
    QCOMPARE(color_spy.count(), 1);
    QCOMPARE(color_spy.at(0).at(0).value<QColor>(), QColor(QStringLiteral("#A349A4")));
}

void PaintBarTest::keepsColorAfterTextProperties()
{
    PaintBar bar;
    bar.setCurrentTool(PaintTool::kText);
    bar.show();
    QCoreApplication::processEvents();

    auto *font = bar.findChild<QWidget *>(QStringLiteral("combo_FontFamily"));
    auto *colorGroup = bar.findChild<QWidget *>(QStringLiteral("container_ColorGroup"));
    QVERIFY(font);
    QVERIFY(colorGroup);
    QVERIFY(font->mapTo(&bar, QPoint()).x() < colorGroup->mapTo(&bar, QPoint()).x());
}

void PaintBarTest::mosaicModesUseButtonsAndSlider()
{
    PaintBar bar;
    bar.setCurrentTool(PaintTool::kMosaic);
    bar.show();
    QCoreApplication::processEvents();

    auto *brush = bar.findChild<QToolButton *>(QStringLiteral("btn_MosaicBrush"));
    auto *area = bar.findChild<QToolButton *>(QStringLiteral("btn_MosaicArea"));
    auto *effectMode = bar.findChild<QToolButton *>(QStringLiteral("btn_EffectMode"));
    auto *level = bar.findChild<QSlider *>(QStringLiteral("slider_EffectLevel"));
    auto *fill = bar.findChild<QToolButton *>(QStringLiteral("btn_StrokeFill"));
    QVERIFY(brush);
    QVERIFY(area);
    QVERIFY(effectMode);
    QVERIFY(level);
    QVERIFY(fill);
    QVERIFY(!bar.findChild<QComboBox *>(QStringLiteral("combo_EffectMode")));
    QVERIFY(brush->isChecked());
    QVERIFY(!area->isChecked());
    QVERIFY(!effectMode->isChecked());
    QVERIFY(brush->mapTo(&bar, QPoint()).x() < area->mapTo(&bar, QPoint()).x());
    QVERIFY(area->mapTo(&bar, QPoint()).x() < effectMode->mapTo(&bar, QPoint()).x());
    QVERIFY(effectMode->mapTo(&bar, QPoint()).x() < level->mapTo(&bar, QPoint()).x());
    QCOMPARE(level->minimum(), 1);
    QCOMPARE(level->maximum(), 64);
    QVERIFY(level->toolTip().contains(QStringLiteral("笔宽")));

    QSignalSpy fillSpy(&bar, &PaintBar::sigFilledChanged);
    QTest::mouseClick(area, Qt::LeftButton);
    QVERIFY(level->toolTip().contains(QStringLiteral("强度")));
    QCOMPARE(fillSpy.count(), 1);
    QCOMPARE(fillSpy.at(0).at(0).toBool(), true);
    QVERIFY(!brush->isChecked());
    QVERIFY(area->isChecked());
    QVERIFY(fill->isChecked());

    QTest::mouseClick(brush, Qt::LeftButton);
    QCOMPARE(fillSpy.count(), 2);
    QCOMPARE(fillSpy.at(1).at(0).toBool(), false);
    QVERIFY(brush->isChecked());
    QVERIFY(!area->isChecked());
    QVERIFY(!fill->isChecked());

    QSignalSpy modeSpy(&bar, &PaintBar::sigEffectModeChanged);
    QSignalSpy levelSpy(&bar, &PaintBar::sigEffectLevelChanged);
    QTest::mouseClick(effectMode, Qt::LeftButton);
    QCOMPARE(modeSpy.count(), 1);
    QCOMPARE(modeSpy.at(0).at(0).toInt(), static_cast<int>(PaintEffectMode::kBlur));
    QVERIFY(effectMode->isChecked());

    QTest::mouseClick(area, Qt::LeftButton);
    level->setValue(19);
    QCOMPARE(levelSpy.count(), 1);
    QCOMPARE(levelSpy.at(0).at(0).toInt(), 19);
}

void PaintBarTest::mosaicBrushSliderControlsPenWidth()
{
    PaintBar bar;
    bar.setCurrentTool(PaintTool::kMosaic);
    bar.show();
    QCoreApplication::processEvents();

    auto *brush = bar.findChild<QToolButton *>(QStringLiteral("btn_MosaicBrush"));
    auto *area = bar.findChild<QToolButton *>(QStringLiteral("btn_MosaicArea"));
    auto *level = bar.findChild<QSlider *>(QStringLiteral("slider_EffectLevel"));
    QVERIFY(brush);
    QVERIFY(area);
    QVERIFY(level);
    QVERIFY(brush->isChecked());
    QVERIFY(level->toolTip().contains(QStringLiteral("笔宽")));

    QSignalSpy pen_width_spy(&bar, &PaintBar::sigPenWidthChanged);
    QSignalSpy effect_level_spy(&bar, &PaintBar::sigEffectLevelChanged);
    level->setValue(19);
    QCOMPARE(pen_width_spy.count(), 1);
    QCOMPARE(pen_width_spy.at(0).at(0).toReal(), 19.0);
    QCOMPARE(effect_level_spy.count(), 0);

    QTest::mouseClick(area, Qt::LeftButton);
    QVERIFY(level->toolTip().contains(QStringLiteral("强度")));
    level->setValue(23);
    QCOMPARE(effect_level_spy.count(), 1);
    QCOMPARE(effect_level_spy.at(0).at(0).toInt(), 23);
    QCOMPARE(pen_width_spy.count(), 1);

    QTest::mouseClick(brush, Qt::LeftButton);
    QCOMPARE(level->value(), 19);
    QVERIFY(level->toolTip().contains(QStringLiteral("笔宽")));
    QCOMPARE(pen_width_spy.count(), 1);
    level->setValue(25);
    QCOMPARE(pen_width_spy.count(), 2);
    QCOMPARE(pen_width_spy.at(1).at(0).toReal(), 25.0);
}

void PaintBarTest::externalPenWidthUpdatesMosaicSlider()
{
    PaintBar bar;
    bar.setCurrentTool(PaintTool::kMosaic);

    auto *level = bar.findChild<QSlider *>(QStringLiteral("slider_EffectLevel"));
    QVERIFY(level);
    bar.setCurrentPenWidth(64);

    QCOMPARE(level->value(), 64);
    QVERIFY(level->toolTip().contains(QStringLiteral("64")));
}

void PaintBarTest::wheelChangesSize()
{
    PaintBar bar;
    bar.setCurrentTool(PaintTool::kRectangle);
    bar.show();
    QCoreApplication::processEvents();

    auto *size = bar.findChild<QToolButton *>(QStringLiteral("btn_PaintSize"));
    QVERIFY(size);
    QSignalSpy size_spy(&bar, &PaintBar::sigPenWidthChanged);

    const QPointF local_position(size->rect().center());
    QWheelEvent first_wheel(local_position,
                            size->mapToGlobal(local_position.toPoint()),
                            QPoint(),
                            QPoint(0, 120),
                            Qt::NoButton,
                            Qt::NoModifier,
                            Qt::NoScrollPhase,
                            false);
    QCoreApplication::sendEvent(size, &first_wheel);
    QCOMPARE(size_spy.count(), 1);

    const qreal first_size = size_spy.at(0).at(0).toReal();
    QWheelEvent second_wheel(local_position,
                             size->mapToGlobal(local_position.toPoint()),
                             QPoint(),
                             QPoint(0, 120),
                             Qt::NoButton,
                             Qt::NoModifier,
                             Qt::NoScrollPhase,
                             false);
    QCoreApplication::sendEvent(size, &second_wheel);
    QCOMPARE(size_spy.count(), 2);
    QVERIFY(size_spy.at(1).at(0).toReal() > first_size);
    QVERIFY(size->toolTip().contains(QStringLiteral("大小")));
}

void PaintBarTest::rendersToolbarLayouts()
{
    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (render_directory.isEmpty()) {
        QSKIP("QSNP_RENDER_DIR is not set");
    }

    PaintBar bar;
    bar.show();
    QCoreApplication::processEvents();
    for (const auto &entry :
         {qMakePair(QStringLiteral("rectangle"), PaintTool::kRectangle),
          qMakePair(QStringLiteral("arrow"), PaintTool::kArrow),
          qMakePair(QStringLiteral("mosaic"), PaintTool::kMosaic),
          qMakePair(QStringLiteral("marker"), PaintTool::kMarker),
          qMakePair(QStringLiteral("text"), PaintTool::kText)}) {
        bar.setCurrentTool(entry.second);
        QCoreApplication::processEvents();
        QImage image(bar.size() + QSize(16, 16), QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor(28, 28, 28));
        QPainter painter(&image);
        bar.render(&painter, QPoint(8, 8));
        painter.end();
        QVERIFY(image.save(render_directory + QStringLiteral("/paint-bar-%1.png").arg(entry.first)));
    }
}

QTEST_MAIN(PaintBarTest)

#include "paintbartest.moc"
