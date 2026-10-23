#include "pinsettings.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

PinSettings::PinSettings(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(16);

    auto *window_group = new QGroupBox(tr("贴图窗口"), this);
    auto *window_layout = new QVBoxLayout(window_group);
    m_shadow = new QCheckBox(tr("显示窗口阴影"), window_group);
    m_shadow->setObjectName(QStringLiteral("check_PinShadow"));
    m_stayOnTop = new QCheckBox(tr("贴图始终置顶"), window_group);
    m_stayOnTop->setObjectName(QStringLiteral("check_PinStayOnTop"));
    window_layout->addWidget(m_shadow);
    window_layout->addWidget(m_stayOnTop);
    layout->addWidget(window_group);

    auto *text_group = new QGroupBox(tr("剪贴板文字转贴图"), this);
    auto *form = new QFormLayout(text_group);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_fontFamily = new QFontComboBox(text_group);
    m_fontFamily->setObjectName(QStringLiteral("combo_TextImageFontFamily"));
    m_fontSize = new QSpinBox(text_group);
    m_fontSize->setObjectName(QStringLiteral("spin_TextImageFontSize"));
    m_fontSize->setRange(6, 96);
    m_fontSize->setSuffix(QStringLiteral(" px"));
    m_maxWidth = new QSpinBox(text_group);
    m_maxWidth->setObjectName(QStringLiteral("spin_TextImageMaxWidth"));
    m_maxWidth->setRange(64, 4096);
    m_maxWidth->setSuffix(QStringLiteral(" px"));
    m_padding = new QSpinBox(text_group);
    m_padding->setObjectName(QStringLiteral("spin_TextImagePadding"));
    m_padding->setRange(0, 128);
    m_padding->setSuffix(QStringLiteral(" px"));

    m_textColor = new QPushButton(text_group);
    m_textColor->setObjectName(QStringLiteral("btn_TextImageColor"));
    m_backgroundColor = new QPushButton(text_group);
    m_backgroundColor->setObjectName(QStringLiteral("btn_TextImageBackground"));
    auto *color_row = new QWidget(text_group);
    auto *color_layout = new QHBoxLayout(color_row);
    color_layout->setContentsMargins(0, 0, 0, 0);
    color_layout->addWidget(m_textColor);
    color_layout->addWidget(m_backgroundColor);
    color_layout->addStretch();

    form->addRow(tr("字体："), m_fontFamily);
    form->addRow(tr("字号："), m_fontSize);
    form->addRow(tr("最大宽度："), m_maxWidth);
    form->addRow(tr("内边距："), m_padding);
    form->addRow(tr("颜色："), color_row);
    layout->addWidget(text_group);
    layout->addStretch();

    connect(m_shadow, &QCheckBox::toggled, this, &PinSettings::sigChanged);
    connect(m_stayOnTop, &QCheckBox::toggled, this, &PinSettings::sigChanged);
    connect(m_fontFamily, &QFontComboBox::currentFontChanged, this, &PinSettings::sigChanged);
    connect(m_fontSize, &QSpinBox::valueChanged, this, &PinSettings::sigChanged);
    connect(m_maxWidth, &QSpinBox::valueChanged, this, &PinSettings::sigChanged);
    connect(m_padding, &QSpinBox::valueChanged, this, &PinSettings::sigChanged);
    connect(m_textColor, &QPushButton::clicked, this, &PinSettings::chooseTextColor);
    connect(m_backgroundColor, &QPushButton::clicked, this, &PinSettings::chooseBackgroundColor);
    updateColorButtons();
}

void PinSettings::setPreferences(const ApplicationPreferences &preferences)
{
    const auto          &settings = preferences.pin;
    const QSignalBlocker shadow_blocker(m_shadow);
    const QSignalBlocker top_blocker(m_stayOnTop);
    const QSignalBlocker font_blocker(m_fontFamily);
    const QSignalBlocker size_blocker(m_fontSize);
    const QSignalBlocker width_blocker(m_maxWidth);
    const QSignalBlocker padding_blocker(m_padding);

    m_shadow->setChecked(settings.showShadow);
    m_stayOnTop->setChecked(settings.stayOnTop);
    m_fontFamily->setCurrentFont(QFont(settings.textToImage.fontFamily));
    m_fontSize->setValue(settings.textToImage.fontSize);
    m_maxWidth->setValue(settings.textToImage.maxWidth);
    m_padding->setValue(settings.textToImage.padding);
    m_currentTextColor = settings.textToImage.textColor;
    m_currentBackgroundColor = settings.textToImage.backgroundColor;
    updateColorButtons();
}

void PinSettings::writePreferences(ApplicationPreferences &preferences) const
{
    preferences.pin.showShadow = m_shadow->isChecked();
    preferences.pin.stayOnTop = m_stayOnTop->isChecked();
    preferences.pin.textToImage.fontFamily = m_fontFamily->currentFont().family();
    preferences.pin.textToImage.fontSize = m_fontSize->value();
    preferences.pin.textToImage.maxWidth = m_maxWidth->value();
    preferences.pin.textToImage.padding = m_padding->value();
    preferences.pin.textToImage.textColor = m_currentTextColor;
    preferences.pin.textToImage.backgroundColor = m_currentBackgroundColor;
}

void PinSettings::restoreDefaults()
{
    ApplicationPreferences preferences;
    setPreferences(preferences);
}

void PinSettings::chooseTextColor()
{
    const QColor selected = QColorDialog::getColor(m_currentTextColor, this, tr("选择文字颜色"));
    if (!selected.isValid()) {
        return;
    }
    m_currentTextColor = selected;
    updateColorButtons();
    emit sigChanged();
}

void PinSettings::chooseBackgroundColor()
{
    const QColor selected = QColorDialog::getColor(m_currentBackgroundColor, this, tr("选择背景颜色"));
    if (!selected.isValid()) {
        return;
    }
    m_currentBackgroundColor = selected;
    updateColorButtons();
    emit sigChanged();
}

void PinSettings::updateColorButtons()
{
    m_textColor->setText(tr("文字 %1").arg(m_currentTextColor.name(QColor::HexRgb).toUpper()));
    m_textColor->setStyleSheet(
        QStringLiteral("QPushButton { text-align:left; padding-left:10px; border-left:18px solid %1; }")
            .arg(m_currentTextColor.name(QColor::HexRgb)));
    m_backgroundColor->setText(tr("背景 %1").arg(m_currentBackgroundColor.name(QColor::HexRgb).toUpper()));
    m_backgroundColor->setStyleSheet(
        QStringLiteral("QPushButton { text-align:left; padding-left:10px; border-left:18px solid %1; }")
            .arg(m_currentBackgroundColor.name(QColor::HexRgb)));
}
