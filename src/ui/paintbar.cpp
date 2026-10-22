#include "paintbar.h"

#include "paint/paintwidths.h"
#include "ui/glyphicon.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QFontComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <array>

namespace {
QString toolObjectName(PaintTool tool)
{
    switch (tool) {
    case PaintTool::kSelect:
        return QStringLiteral("btn_ToolSelect");
    case PaintTool::kRectangle:
        return QStringLiteral("btn_ToolRectangle");
    case PaintTool::kEllipse:
        return QStringLiteral("btn_ToolEllipse");
    case PaintTool::kLine:
        return QStringLiteral("btn_ToolLine");
    case PaintTool::kArrow:
        return QStringLiteral("btn_ToolArrow");
    case PaintTool::kPencil:
        return QStringLiteral("btn_ToolPencil");
    case PaintTool::kMarker:
        return QStringLiteral("btn_ToolMarker");
    case PaintTool::kMosaic:
        return QStringLiteral("btn_ToolMosaic");
    case PaintTool::kText:
        return QStringLiteral("btn_ToolText");
    case PaintTool::kEraser:
        return QStringLiteral("btn_ToolEraser");
    case PaintTool::kNumber:
        return QStringLiteral("btn_ToolNumber");
    }
    return QStringLiteral("btn_Tool");
}

QToolButton *createIconButton(GlyphIcon glyph, const QString &toolTip, QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setIcon(createGlyphIcon(glyph));
    button->setIconSize(QSize(20, 20));
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setToolTip(toolTip);
    button->setFixedSize(32, 32);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

QIcon createSizeDotIcon(qreal size, bool is_text)
{
    QIcon icon;
    const qreal radius = is_text ? qBound<qreal>(2.5, 2.5 + (size - 8.0) / 18.0, 9.0)
                                 : qBound<qreal>(2.0, 2.0 + size * 0.45, 10.0);
    for (const int icon_size : {16, 20, 24, 32}) {
        QPixmap pixmap(icon_size, icon_size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.drawEllipse(QPointF(icon_size / 2.0, icon_size / 2.0), radius, radius);
        icon.addPixmap(pixmap);
    }
    return icon;
}

std::array<int, 12> fontSizeSteps()
{
    return {8, 10, 12, 16, 20, 24, 32, 40, 48, 60, 72, 96};
}

std::array<QColor, 20> paletteColors()
{
    return {QColor(QStringLiteral("#000000")),
            QColor(QStringLiteral("#7F7F7F")),
            QColor(QStringLiteral("#880015")),
            QColor(QStringLiteral("#ED1C24")),
            QColor(QStringLiteral("#FF7F27")),
            QColor(QStringLiteral("#FFF200")),
            QColor(QStringLiteral("#22B14C")),
            QColor(QStringLiteral("#00A2E8")),
            QColor(QStringLiteral("#3F48CC")),
            QColor(QStringLiteral("#A349A4")),
            QColor(QStringLiteral("#FFFFFF")),
            QColor(QStringLiteral("#C3C3C3")),
            QColor(QStringLiteral("#B97A57")),
            QColor(QStringLiteral("#FFAEC9")),
            QColor(QStringLiteral("#FFC90E")),
            QColor(QStringLiteral("#EFE4B0")),
            QColor(QStringLiteral("#B5E61D")),
            QColor(QStringLiteral("#99D9EA")),
            QColor(QStringLiteral("#7092BE")),
            QColor(QStringLiteral("#C8BFE7"))};
}
} // namespace

PaintBar::PaintBar(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(QStringLiteral("QToolButton {"
                                 " color: #E8E8E8; background: transparent; border: none;"
                                 " border-radius: 4px; font-size: 12px;"
                                 "}"
                                 "QToolButton:hover { background: #3C3C3C; }"
                                 "QToolButton:checked { background: #0A84FF; color: white; }"
                                 "QToolButton:disabled { color: #777777; }"
                                 "QLabel { color: #B8B8B8; font-size: 11px; }"
                                 "QComboBox, QFontComboBox, QSpinBox {"
                                 " color: #E8E8E8; background: #3B3B3B; border: 1px solid #555;"
                                 " border-radius: 3px; min-height: 22px; padding: 0 4px; font-size: 11px;"
                                 "}"
                                 "QSlider::groove:horizontal {"
                                 " height: 4px; background: #555; border-radius: 2px;"
                                 "}"
                                 "QSlider::sub-page:horizontal { background: #0A84FF; border-radius: 2px; }"
                                 "QSlider::handle:horizontal {"
                                 " width: 12px; height: 12px; margin: -5px 0; background: #E8E8E8;"
                                 " border: 1px solid #666; border-radius: 6px;"
                                 "}"));

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(2);

    m_primaryRow = new QWidget(this);
    m_primaryRow->setObjectName(QStringLiteral("row_PaintTools"));
    m_primaryLayout = new QHBoxLayout(m_primaryRow);
    m_primaryLayout->setContentsMargins(5, 4, 5, 4);
    m_primaryLayout->setSpacing(2);
    outerLayout->addWidget(m_primaryRow);

    addToolButton(GlyphIcon::kRectangle, QStringLiteral("矩形"), PaintTool::kRectangle);
    addToolButton(GlyphIcon::kEllipse, QStringLiteral("椭圆"), PaintTool::kEllipse);
    addPrimarySeparator(QStringLiteral("separator_Shapes"));
    addToolButton(GlyphIcon::kLine, QStringLiteral("折线 Ctrl+2"), PaintTool::kLine);
    addToolButton(GlyphIcon::kArrow, QStringLiteral("箭头 Ctrl+3"), PaintTool::kArrow);
    addPrimarySeparator(QStringLiteral("separator_Lines"));
    addToolButton(GlyphIcon::kPencil, QStringLiteral("画笔 Ctrl+4"), PaintTool::kPencil);
    addToolButton(GlyphIcon::kMarker, QStringLiteral("记号笔 Ctrl+5"), PaintTool::kMarker);
    addToolButton(GlyphIcon::kEraser, QStringLiteral("橡皮擦 E"), PaintTool::kEraser);
    addPrimarySeparator(QStringLiteral("separator_Freehand"));
    addToolButton(GlyphIcon::kMosaic, QStringLiteral("马赛克/模糊 Ctrl+6"), PaintTool::kMosaic);
    addToolButton(GlyphIcon::kText, QStringLiteral("文字 Ctrl+7"), PaintTool::kText);
    addToolButton(GlyphIcon::kNumber, QStringLiteral("序号"), PaintTool::kNumber);
    addPrimarySeparator(QStringLiteral("separator_Tools"));

    m_undoButton = addCommandButton(GlyphIcon::kUndo,
                                    QStringLiteral("撤销 Ctrl+Z"),
                                    QStringLiteral("btn_Undo"),
                                    CaptureCommand::kUndo);
    m_redoButton = addCommandButton(GlyphIcon::kRedo,
                                    QStringLiteral("重做 Ctrl+Y"),
                                    QStringLiteral("btn_Redo"),
                                    CaptureCommand::kRedo);
    m_undoButton->setEnabled(false);
    m_redoButton->setEnabled(false);
    addPrimarySeparator(QStringLiteral("separator_History"));
    addCommandButton(GlyphIcon::kCopy, QStringLiteral("复制 Ctrl+C"), QStringLiteral("btn_Copy"), CaptureCommand::kCopy);
    addCommandButton(GlyphIcon::kSave, QStringLiteral("保存 Ctrl+S"), QStringLiteral("btn_Save"), CaptureCommand::kSave);
    addCommandButton(GlyphIcon::kPin,
                     QStringLiteral("贴到屏幕 Ctrl+T"),
                     QStringLiteral("btn_Pin"),
                     CaptureCommand::kPin);
    addPrimarySeparator(QStringLiteral("separator_Output"));

    auto *moreButton = new QToolButton(m_primaryRow);
    moreButton->setObjectName(QStringLiteral("btn_MoreActions"));
    moreButton->setIcon(createGlyphIcon(GlyphIcon::kOptions));
    moreButton->setIconSize(QSize(20, 20));
    moreButton->setToolTip(QStringLiteral("更多操作"));
    moreButton->setFixedSize(32, 32);
    moreButton->setFocusPolicy(Qt::NoFocus);
    moreButton->setPopupMode(QToolButton::InstantPopup);
    auto *moreMenu = new QMenu(moreButton);
    moreMenu->setStyleSheet(QStringLiteral("QMenu { background: #292929; color: #E8E8E8; border: 1px solid #555;"
                                           " padding: 5px; }"
                                           "QMenu::item { padding: 6px 20px 6px 10px; border-radius: 4px; }"
                                           "QMenu::item:selected { background: #0A84FF; color: white; }"));
    auto *clearAction = moreMenu->addAction(createGlyphIcon(GlyphIcon::kClear), QStringLiteral("清空标注"));
    clearAction->setObjectName(QStringLiteral("act_ClearAnnotations"));
    connect(clearAction, &QAction::triggered, this, [this]() { emit sigCommandRequested(CaptureCommand::kClearAll); });
    moreButton->setMenu(moreMenu);
    m_primaryLayout->addWidget(moreButton);

    addCommandButton(GlyphIcon::kClose,
                     QStringLiteral("退出截图 Esc"),
                     QStringLiteral("btn_Close"),
                     CaptureCommand::kCancel);
    m_primaryLayout->addStretch();

    m_secondaryRow = new QWidget(this);
    m_secondaryRow->setObjectName(QStringLiteral("row_PaintProperties"));
    m_secondaryLayout = new QHBoxLayout(m_secondaryRow);
    m_secondaryLayout->setContentsMargins(5, 4, 5, 4);
    m_secondaryLayout->setSpacing(3);
    outerLayout->addWidget(m_secondaryRow);

    m_sizeButton = new QToolButton(m_secondaryRow);
    m_sizeButton->setObjectName(QStringLiteral("btn_PaintSize"));
    m_sizeButton->setIconSize(QSize(20, 20));
    m_sizeButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_sizeButton->setFixedSize(32, 32);
    m_sizeButton->setFocusPolicy(Qt::NoFocus);
    m_sizeButton->installEventFilter(this);
    m_secondaryLayout->addWidget(m_sizeButton);

    m_fillButton = createIconButton(GlyphIcon::kFill, QStringLiteral("填充/描边"), this);
    m_fillButton->setObjectName(QStringLiteral("btn_StrokeFill"));
    m_fillButton->setCheckable(true);
    connect(m_fillButton, &QToolButton::toggled, this, &PaintBar::sigFilledChanged);
    m_secondaryLayout->addWidget(m_fillButton);

    m_mosaicModeGroup = new QButtonGroup(this);
    m_mosaicModeGroup->setExclusive(true);
    m_mosaicBrushButton = createIconButton(GlyphIcon::kMosaicBrush,
                                           QStringLiteral("画笔马赛克"),
                                           m_secondaryRow);
    m_mosaicBrushButton->setObjectName(QStringLiteral("btn_MosaicBrush"));
    m_mosaicBrushButton->setCheckable(true);
    m_mosaicModeGroup->addButton(m_mosaicBrushButton, 0);
    m_secondaryLayout->addWidget(m_mosaicBrushButton);

    m_mosaicAreaButton = createIconButton(GlyphIcon::kMosaicArea,
                                          QStringLiteral("框选区域马赛克"),
                                          m_secondaryRow);
    m_mosaicAreaButton->setObjectName(QStringLiteral("btn_MosaicArea"));
    m_mosaicAreaButton->setCheckable(true);
    m_mosaicModeGroup->addButton(m_mosaicAreaButton, 1);
    m_secondaryLayout->addWidget(m_mosaicAreaButton);
    m_mosaicBrushButton->setChecked(true);
    connect(m_mosaicModeGroup, &QButtonGroup::idClicked, this, [this](int id) {
        setMosaicBrushMode(id == 0);
        emit sigFilledChanged(!m_mosaicBrushMode);
    });

    m_opacityContainer = new QWidget(m_secondaryRow);
    m_opacityContainer->setObjectName(QStringLiteral("container_MarkerOpacity"));
    auto *opacityLayout = new QHBoxLayout(m_opacityContainer);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacityLayout->setSpacing(3);
    opacityLayout->addWidget(new QLabel(QStringLiteral("透明度"), m_opacityContainer));
    m_markerOpacitySpin = new QSpinBox(m_opacityContainer);
    m_markerOpacitySpin->setObjectName(QStringLiteral("spin_MarkerOpacity"));
    m_markerOpacitySpin->setRange(10, 100);
    m_markerOpacitySpin->setValue(43);
    m_markerOpacitySpin->setSuffix(QStringLiteral("%"));
    m_markerOpacitySpin->setFixedWidth(64);
    m_markerOpacitySpin->setFocusPolicy(Qt::NoFocus);
    opacityLayout->addWidget(m_markerOpacitySpin);
    connect(m_markerOpacitySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        emit sigMarkerOpacityChanged(qRound(value * 2.55));
    });
    m_secondaryLayout->addWidget(m_opacityContainer);

    m_commonSeparator = addSecondarySeparator(QStringLiteral("separator_CommonProperties"));

    m_arrowStyleGroup = new QButtonGroup(this);
    m_arrowStyleGroup->setExclusive(true);
    const auto addArrowStyleButton = [this](GlyphIcon glyph,
                                            const QString &toolTip,
                                            const QString &objectName,
                                            ArrowStyle style) {
        auto *button = createIconButton(glyph, toolTip, m_secondaryRow);
        button->setObjectName(objectName);
        button->setCheckable(true);
        m_arrowStyleGroup->addButton(button, static_cast<int>(style));
        m_arrowStyleButtons.append(button);
        m_secondaryLayout->addWidget(button);
    };
    addArrowStyleButton(GlyphIcon::kArrowSolid,
                        QStringLiteral("实心箭头"),
                        QStringLiteral("btn_ArrowSolid"),
                        ArrowStyle::kSolid);
    addArrowStyleButton(GlyphIcon::kArrowSolidTail,
                        QStringLiteral("实心尾线箭头"),
                        QStringLiteral("btn_ArrowSolidTail"),
                        ArrowStyle::kSolidTail);
    addArrowStyleButton(GlyphIcon::kArrowHollowTail,
                        QStringLiteral("空心尾线箭头"),
                        QStringLiteral("btn_ArrowHollowTail"),
                        ArrowStyle::kHollowTail);
    m_arrowStyleGroup->button(static_cast<int>(ArrowStyle::kSolid))->setChecked(true);
    connect(m_arrowStyleGroup, &QButtonGroup::idClicked, this, [this](int id) {
        emit sigArrowStyleChanged(static_cast<ArrowStyle>(id));
    });

    m_effectModeButton = createIconButton(GlyphIcon::kMosaic,
                                          QStringLiteral("当前：马赛克（点击切换模糊）"),
                                          m_secondaryRow);
    m_effectModeButton->setObjectName(QStringLiteral("btn_EffectMode"));
    m_effectModeButton->setCheckable(true);
    m_secondaryLayout->addWidget(m_effectModeButton);

    m_effectLevelSlider = new QSlider(Qt::Horizontal, m_secondaryRow);
    m_effectLevelSlider->setObjectName(QStringLiteral("slider_EffectLevel"));
    m_effectLevelSlider->setRange(1, 64);
    m_effectLevelSlider->setValue(m_mosaicLevel);
    m_effectLevelSlider->setFixedWidth(100);
    m_effectLevelSlider->setFocusPolicy(Qt::NoFocus);
    m_effectLevelSlider->setAccessibleName(QStringLiteral("效果强度"));
    m_secondaryLayout->addWidget(m_effectLevelSlider);
    connect(m_effectLevelSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_mosaicBrushMode) {
            m_size = value;
            updateSizeButton();
            updateEffectLevelToolTip();
            emit sigPenWidthChanged(m_size);
            return;
        }
        if (m_effectMode == PaintEffectMode::kMosaic) {
            m_mosaicLevel = value;
        } else {
            m_blurLevel = value;
        }
        updateEffectLevelToolTip();
        emit sigEffectLevelChanged(value);
    });
    connect(m_effectModeButton, &QToolButton::toggled, this, [this](bool blurred) {
        m_effectMode = blurred ? PaintEffectMode::kBlur : PaintEffectMode::kMosaic;
        updateEffectModeButton();
        updateEffectSlider();
        emit sigEffectModeChanged(m_effectMode);
    });
    updateEffectModeButton();
    updateEffectSlider();

    m_fontCombo = new QFontComboBox(m_secondaryRow);
    m_fontCombo->setObjectName(QStringLiteral("combo_FontFamily"));
    m_fontCombo->setFocusPolicy(Qt::NoFocus);
    m_fontCombo->setFixedWidth(132);
    m_secondaryLayout->addWidget(m_fontCombo);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        emit sigFontFamilyChanged(font.family());
    });

    m_boldButton = createIconButton(GlyphIcon::kBold, QStringLiteral("粗体"), this);
    m_boldButton->setObjectName(QStringLiteral("btn_TextBold"));
    m_boldButton->setCheckable(true);
    connect(m_boldButton, &QToolButton::toggled, this, &PaintBar::sigFontBoldChanged);
    m_secondaryLayout->addWidget(m_boldButton);

    m_italicButton = createIconButton(GlyphIcon::kItalic, QStringLiteral("斜体"), this);
    m_italicButton->setObjectName(QStringLiteral("btn_TextItalic"));
    m_italicButton->setCheckable(true);
    connect(m_italicButton, &QToolButton::toggled, this, &PaintBar::sigFontItalicChanged);
    m_secondaryLayout->addWidget(m_italicButton);

    m_outlineButton = createIconButton(GlyphIcon::kTextOutline, QStringLiteral("文字描边"), this);
    m_outlineButton->setObjectName(QStringLiteral("btn_TextOutline"));
    m_outlineButton->setCheckable(true);
    connect(m_outlineButton, &QToolButton::toggled, this, &PaintBar::sigTextOutlinedChanged);
    m_secondaryLayout->addWidget(m_outlineButton);

    m_textBackgroundButton = createIconButton(GlyphIcon::kTextBackground, QStringLiteral("文字背景"), this);
    m_textBackgroundButton->setObjectName(QStringLiteral("btn_TextBackground"));
    m_textBackgroundButton->setCheckable(true);
    connect(m_textBackgroundButton, &QToolButton::toggled, this, &PaintBar::sigTextBackgroundChanged);
    m_secondaryLayout->addWidget(m_textBackgroundButton);

    m_colorSeparator = addSecondarySeparator(QStringLiteral("separator_ColorGroup"));
    m_colorGroup = new QWidget(m_secondaryRow);
    m_colorGroup->setObjectName(QStringLiteral("container_ColorGroup"));
    auto *colorGroupLayout = new QHBoxLayout(m_colorGroup);
    colorGroupLayout->setContentsMargins(0, 0, 0, 0);
    colorGroupLayout->setSpacing(4);

    m_colorButton = new QToolButton(m_colorGroup);
    m_colorButton->setObjectName(QStringLiteral("btn_PaintColor"));
    m_colorButton->setToolTip(QStringLiteral("自定义颜色"));
    m_colorButton->setIcon(createColorSwatchIcon(m_color));
    m_colorButton->setIconSize(QSize(28, 28));
    m_colorButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_colorButton->setFixedSize(42, 38);
    m_colorButton->setFocusPolicy(Qt::NoFocus);
    connect(m_colorButton, &QToolButton::clicked, this, [this]() {
        const QColor selected = QColorDialog::getColor(m_color, this, QStringLiteral("选择颜色"));
        if (selected.isValid()) {
            setCurrentColor(selected);
            emit sigColorChanged(selected);
        }
    });
    colorGroupLayout->addWidget(m_colorButton);

    m_paletteContainer = new QWidget(m_colorGroup);
    m_paletteContainer->setObjectName(QStringLiteral("container_ColorPalette"));
    m_paletteContainer->setFixedHeight(38);
    auto *paletteLayout = new QGridLayout(m_paletteContainer);
    paletteLayout->setContentsMargins(0, 0, 0, 0);
    paletteLayout->setSpacing(2);
    const auto colors = paletteColors();
    for (int index = 0; index < static_cast<int>(colors.size()); ++index) {
        const QColor color = colors[index];
        auto *button = new QToolButton(m_paletteContainer);
        button->setObjectName(QStringLiteral("btn_PaletteColor%1").arg(index + 1, 2, 10, QChar('0')));
        button->setProperty("paletteColor", QVariant::fromValue(color));
        button->setToolTip(color.name(QColor::HexRgb).toUpper());
        button->setCheckable(true);
        button->setAutoRaise(false);
        button->setFixedSize(18, 18);
        button->setFocusPolicy(Qt::NoFocus);
        button->setStyleSheet(QStringLiteral(
                                  "QToolButton { background-color: %1; border: 2px solid #D6D6D6; border-radius: 1px; }"
                                  "QToolButton:hover { border-color: white; }"
                                  "QToolButton:checked { background-color: %1; border: 2px solid #0A84FF; }")
                                  .arg(color.name(QColor::HexRgb)));
        paletteLayout->addWidget(button, index / 10, index % 10);
        m_paletteButtons.append(button);
        connect(button, &QToolButton::clicked, this, [this, color]() {
            setCurrentColor(color);
            emit sigColorChanged(color);
        });
    }
    colorGroupLayout->addWidget(m_paletteContainer);
    m_secondaryLayout->addWidget(m_colorGroup);
    m_secondaryLayout->addStretch();

    setCurrentTool(PaintTool::kRectangle);
    updateSizeButton();
    refreshSize();
}

PaintBar::~PaintBar() = default;

void PaintBar::setCurrentTool(PaintTool tool)
{
    m_tool = tool;
    for (QToolButton *button : m_toolButtons) {
        const auto button_tool = static_cast<PaintTool>(button->property("paintTool").toInt());
        button->setChecked(button_tool == tool);
    }
    updateSizeButton();
    updateSecondaryControls();
    refreshSize();
}

void PaintBar::setCurrentColor(const QColor &color)
{
    if (!color.isValid()) {
        return;
    }
    m_color = color;
    m_colorButton->setIcon(createColorSwatchIcon(color));
    updatePaletteSelection();
}

void PaintBar::setCurrentPenWidth(qreal width)
{
    if (width <= 0) {
        return;
    }
    m_size = width;
    updateSizeButton();
    if (m_tool == PaintTool::kMosaic && m_mosaicBrushMode) {
        updateEffectSlider();
    }
}

void PaintBar::setUndoEnabled(bool enabled)
{
    m_undoButton->setEnabled(enabled);
}

void PaintBar::setRedoEnabled(bool enabled)
{
    m_redoButton->setEnabled(enabled);
}

bool PaintBar::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_sizeButton && event->type() == QEvent::Wheel) {
        auto *wheelEvent = static_cast<QWheelEvent *>(event);
        if (wheelEvent->angleDelta().y() != 0) {
            adjustSizeByWheel(wheelEvent->angleDelta().y() > 0 ? 1 : -1);
            wheelEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PaintBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(88, 88, 88), 1));
    painter.setBrush(QColor(43, 43, 43, 240));
    for (QWidget *row : {m_primaryRow, m_secondaryRow}) {
        if (row != nullptr && row->isVisible()) {
            painter.drawRoundedRect(QRectF(row->geometry()).adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
        }
    }
}

QToolButton *PaintBar::addCommandButton(GlyphIcon      glyph,
                                        const QString &toolTip,
                                        const QString &objectName,
                                        CaptureCommand command)
{
    auto *button = createIconButton(glyph, toolTip, m_primaryRow);
    button->setObjectName(objectName);
    m_primaryLayout->addWidget(button);
    connect(button, &QToolButton::clicked, this, [this, command]() { emit sigCommandRequested(command); });
    return button;
}

QToolButton *PaintBar::addToolButton(GlyphIcon glyph, const QString &toolTip, PaintTool tool)
{
    auto *button = createIconButton(glyph, toolTip, m_primaryRow);
    button->setObjectName(toolObjectName(tool));
    button->setCheckable(true);
    button->setProperty("paintTool", static_cast<int>(tool));
    m_primaryLayout->addWidget(button);
    m_toolButtons.append(button);
    connect(button, &QToolButton::clicked, this, [this, tool]() {
        setCurrentTool(tool);
        if (tool == PaintTool::kMosaic) {
            setMosaicBrushMode(m_mosaicBrushMode);
            emit sigFilledChanged(!m_mosaicBrushMode);
        }
        emit sigToolChanged(tool);
    });
    return button;
}

QLabel *PaintBar::addSecondaryLabel(const QString &text)
{
    auto *label = new QLabel(text, m_secondaryRow);
    m_secondaryLayout->addWidget(label);
    return label;
}

void PaintBar::addPrimarySeparator(const QString &objectName)
{
    auto *separator = new QFrame(m_primaryRow);
    separator->setObjectName(objectName);
    separator->setFixedSize(1, 20);
    separator->setStyleSheet(QStringLiteral("background-color: #555; border: none;"));
    m_primaryLayout->addWidget(separator);
}

QWidget *PaintBar::addSecondarySeparator(const QString &objectName)
{
    auto *separator = new QFrame(m_secondaryRow);
    separator->setObjectName(objectName);
    separator->setFixedSize(1, 20);
    separator->setStyleSheet(QStringLiteral("background-color: #555; border: none;"));
    m_secondaryLayout->addWidget(separator);
    return separator;
}

void PaintBar::updateSecondaryControls()
{
    const bool is_arrow = m_tool == PaintTool::kArrow;
    const bool is_marker = m_tool == PaintTool::kMarker;
    const bool is_effect = m_tool == PaintTool::kMosaic;
    const bool is_text = m_tool == PaintTool::kText;
    const bool is_shape = m_tool == PaintTool::kRectangle || m_tool == PaintTool::kEllipse;
    const bool has_size = !is_effect;
    const bool has_color = !is_effect && m_tool != PaintTool::kEraser;

    m_sizeButton->setVisible(has_size);
    m_fillButton->setVisible(is_shape);
    m_mosaicBrushButton->setVisible(is_effect);
    m_mosaicAreaButton->setVisible(is_effect);
    if (is_effect) {
        setMosaicBrushMode(m_mosaicBrushMode);
    }
    m_opacityContainer->setVisible(is_marker);
    m_colorGroup->setVisible(has_color);
    m_commonSeparator->setVisible(is_arrow || is_effect || is_text);
    for (QToolButton *button : m_arrowStyleButtons) {
        button->setVisible(is_arrow);
    }
    m_effectModeButton->setVisible(is_effect);
    m_effectLevelSlider->setVisible(is_effect);
    m_fontCombo->setVisible(is_text);
    m_boldButton->setVisible(is_text);
    m_italicButton->setVisible(is_text);
    m_outlineButton->setVisible(is_text);
    m_textBackgroundButton->setVisible(is_text);
    m_colorSeparator->setVisible(has_color);
}

void PaintBar::updateSizeButton()
{
    const bool is_text = m_tool == PaintTool::kText;
    const qreal value = is_text ? m_fontSize : m_size;
    m_sizeButton->setIcon(createSizeDotIcon(value, is_text));
    m_sizeButton->setToolTip(is_text ? QStringLiteral("字号：%1 px（滚轮调整）").arg(m_fontSize)
                                    : QStringLiteral("大小：%1 px（滚轮调整）").arg(m_size, 0, 'g', 3));
}

void PaintBar::updatePaletteSelection()
{
    for (QToolButton *button : m_paletteButtons) {
        button->setChecked(button->property("paletteColor").value<QColor>() == m_color);
    }
}

void PaintBar::updateEffectModeButton()
{
    const bool blurred = m_effectMode == PaintEffectMode::kBlur;
    m_effectModeButton->setChecked(blurred);
    m_effectModeButton->setIcon(createGlyphIcon(blurred ? GlyphIcon::kBlur : GlyphIcon::kMosaic));
    m_effectModeButton->setToolTip(blurred ? QStringLiteral("当前：模糊（点击切换马赛克）")
                                           : QStringLiteral("当前：马赛克（点击切换模糊）"));
}

void PaintBar::updateEffectSlider()
{
    int value = m_mosaicLevel;
    if (m_mosaicBrushMode) {
        value = qBound(1, qRound(m_size), m_effectLevelSlider->maximum());
    } else if (m_effectMode == PaintEffectMode::kBlur) {
        value = m_blurLevel;
    }
    const QSignalBlocker blocker(m_effectLevelSlider);
    m_effectLevelSlider->setValue(value);
    updateEffectLevelToolTip();
}

void PaintBar::updateEffectLevelToolTip()
{
    if (m_mosaicBrushMode) {
        m_effectLevelSlider->setAccessibleName(QStringLiteral("笔宽"));
        m_effectLevelSlider->setToolTip(
            QStringLiteral("笔宽：%1 px").arg(m_effectLevelSlider->value()));
        return;
    }
    const QString mode = m_effectMode == PaintEffectMode::kBlur ? QStringLiteral("模糊")
                                                                : QStringLiteral("马赛克");
    m_effectLevelSlider->setAccessibleName(QStringLiteral("效果强度"));
    m_effectLevelSlider->setToolTip(QStringLiteral("%1强度：%2").arg(mode).arg(m_effectLevelSlider->value()));
}

void PaintBar::setMosaicBrushMode(bool brushMode)
{
    m_mosaicBrushMode = brushMode;
    m_mosaicBrushButton->setChecked(brushMode);
    m_mosaicAreaButton->setChecked(!brushMode);
    const QSignalBlocker blocker(m_fillButton);
    m_fillButton->setChecked(!brushMode);
    updateEffectSlider();
}

void PaintBar::adjustSizeByWheel(int direction)
{
    if (m_tool == PaintTool::kText) {
        const auto steps = fontSizeSteps();
        int        nearest = 0;
        for (int index = 1; index < static_cast<int>(steps.size()); ++index) {
            if (qAbs(steps[index] - m_fontSize) < qAbs(steps[nearest] - m_fontSize)) {
                nearest = index;
            }
        }
        nearest = qBound(0, nearest + direction, static_cast<int>(steps.size()) - 1);
        m_fontSize = steps[nearest];
        updateSizeButton();
        emit sigFontSizeChanged(m_fontSize);
        return;
    }

    const auto steps = penSizeSteps();
    int        nearest = 0;
    for (int index = 1; index < static_cast<int>(steps.size()); ++index) {
        if (qAbs(steps[index] - m_size) < qAbs(steps[nearest] - m_size)) {
            nearest = index;
        }
    }
    nearest = qBound(0, nearest + direction, static_cast<int>(steps.size()) - 1);
    m_size = steps[nearest];
    updateSizeButton();
    emit sigPenWidthChanged(m_size);
}

void PaintBar::refreshSize()
{
    adjustSize();
    setFixedSize(sizeHint());
}
