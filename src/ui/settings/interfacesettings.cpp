#include "interfacesettings.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

InterfaceSettings::InterfaceSettings(QWidget *parent)
    : QWidget(parent)
{
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 22, 24, 22);
    pageLayout->setSpacing(16);

    auto *captureBarGroup = new QGroupBox(tr("桌面功能工具条"), this);
    auto *formLayout = new QFormLayout(captureBarGroup);

    m_enableCaptureBar = new QCheckBox(tr("启用功能工具条"), captureBarGroup);
    m_enableCaptureBar->setObjectName(QStringLiteral("check_EnableCaptureBar"));
    formLayout->addRow(m_enableCaptureBar);

    m_autoHide = new QCheckBox(tr("鼠标离开后自动隐藏"), captureBarGroup);
    m_autoHide->setObjectName(QStringLiteral("check_AutoHideCaptureBar"));
    formLayout->addRow(m_autoHide);

    m_snapDistance = new QSpinBox(captureBarGroup);
    m_snapDistance->setObjectName(QStringLiteral("spin_CaptureBarSnapDistance"));
    m_snapDistance->setRange(0, 48);
    m_snapDistance->setSuffix(tr(" 像素"));
    formLayout->addRow(tr("边缘吸附距离："), m_snapDistance);

    m_hotspotWidth = new QSpinBox(captureBarGroup);
    m_hotspotWidth->setObjectName(QStringLiteral("spin_CaptureBarHotspotWidth"));
    m_hotspotWidth->setRange(2, 12);
    m_hotspotWidth->setSuffix(tr(" 像素"));
    formLayout->addRow(tr("唤回热区厚度："), m_hotspotWidth);

    m_hotspotLength = new QSpinBox(captureBarGroup);
    m_hotspotLength->setObjectName(QStringLiteral("spin_CaptureBarHotspotLength"));
    m_hotspotLength->setRange(20, 320);
    m_hotspotLength->setSuffix(tr(" 像素"));
    formLayout->addRow(tr("唤回热区长度："), m_hotspotLength);

    m_hideDelay = new QSpinBox(captureBarGroup);
    m_hideDelay->setObjectName(QStringLiteral("spin_CaptureBarHideDelay"));
    m_hideDelay->setRange(100, 3000);
    m_hideDelay->setSingleStep(100);
    m_hideDelay->setSuffix(tr(" 毫秒"));
    formLayout->addRow(tr("自动隐藏延迟："), m_hideDelay);

    m_revealDelay = new QSpinBox(captureBarGroup);
    m_revealDelay->setObjectName(QStringLiteral("spin_CaptureBarRevealDelay"));
    m_revealDelay->setRange(0, 1000);
    m_revealDelay->setSingleStep(50);
    m_revealDelay->setSuffix(tr(" 毫秒"));
    formLayout->addRow(tr("悬停展开延迟："), m_revealDelay);

    m_slideDuration = new QSpinBox(captureBarGroup);
    m_slideDuration->setObjectName(QStringLiteral("spin_CaptureBarSlideDuration"));
    m_slideDuration->setRange(0, 1000);
    m_slideDuration->setSingleStep(50);
    m_slideDuration->setSuffix(tr(" 毫秒"));
    formLayout->addRow(tr("滑入滑出时长："), m_slideDuration);

    auto *hint = new QLabel(tr("将工具条拖到屏幕边缘即可重新停靠，位置会自动保存。"), captureBarGroup);
    hint->setObjectName(QStringLiteral("label_CaptureBarPlacementHint"));
    hint->setWordWrap(true);
    formLayout->addRow(hint);

    pageLayout->addWidget(captureBarGroup);

    auto *resetPlacementButton = new QPushButton(tr("重置工具条位置"), this);
    resetPlacementButton->setObjectName(QStringLiteral("btn_ResetCaptureBarPlacement"));
    auto *resetRow = new QHBoxLayout();
    resetRow->addWidget(resetPlacementButton);
    resetRow->addStretch();
    pageLayout->addLayout(resetRow);
    pageLayout->addStretch();

    connect(m_enableCaptureBar, &QCheckBox::toggled, this, &InterfaceSettings::sigChanged);
    connect(m_autoHide, &QCheckBox::toggled, this, &InterfaceSettings::sigChanged);
    connect(m_snapDistance, &QSpinBox::valueChanged, this, &InterfaceSettings::sigChanged);
    connect(m_hotspotWidth, &QSpinBox::valueChanged, this, &InterfaceSettings::sigChanged);
    connect(m_hotspotLength, &QSpinBox::valueChanged, this, &InterfaceSettings::sigChanged);
    connect(m_hideDelay, &QSpinBox::valueChanged, this, &InterfaceSettings::sigChanged);
    connect(m_revealDelay, &QSpinBox::valueChanged, this, &InterfaceSettings::sigChanged);
    connect(m_slideDuration, &QSpinBox::valueChanged, this, &InterfaceSettings::sigChanged);
    connect(resetPlacementButton, &QPushButton::clicked, this, [this]() {
        m_resetPlacementRequested = true;
        emit sigChanged();
    });
}

InterfaceSettings::~InterfaceSettings() = default;

void InterfaceSettings::setPreferences(const ApplicationPreferences &preferences)
{
    const QSignalBlocker enableBlocker(m_enableCaptureBar);
    const QSignalBlocker hideBlocker(m_autoHide);
    const QSignalBlocker snapBlocker(m_snapDistance);
    const QSignalBlocker hotspotBlocker(m_hotspotWidth);
    const QSignalBlocker hotspotLengthBlocker(m_hotspotLength);
    const QSignalBlocker delayBlocker(m_hideDelay);
    const QSignalBlocker revealBlocker(m_revealDelay);
    const QSignalBlocker slideBlocker(m_slideDuration);

    const auto &data = preferences.captureBar;
    m_enableCaptureBar->setChecked(data.enabled);
    m_autoHide->setChecked(data.autoHide);
    m_snapDistance->setValue(data.snapDistancePx);
    m_hotspotWidth->setValue(data.hotspotWidthPx);
    m_hotspotLength->setValue(data.hotspotLengthPx);
    m_hideDelay->setValue(data.hideDelayMs);
    m_revealDelay->setValue(data.revealDelayMs);
    m_slideDuration->setValue(data.slideDurationMs);
    m_resetPlacementRequested = false;
}

void InterfaceSettings::writePreferences(ApplicationPreferences &preferences) const
{
    preferences.captureBar.enabled = m_enableCaptureBar->isChecked();
    preferences.captureBar.autoHide = m_autoHide->isChecked();
    preferences.captureBar.snapDistancePx = m_snapDistance->value();
    preferences.captureBar.hotspotWidthPx = m_hotspotWidth->value();
    preferences.captureBar.hotspotLengthPx = m_hotspotLength->value();
    preferences.captureBar.hideDelayMs = m_hideDelay->value();
    preferences.captureBar.revealDelayMs = m_revealDelay->value();
    preferences.captureBar.slideDurationMs = m_slideDuration->value();
    if (m_resetPlacementRequested) {
        const CaptureBarSettingsData defaults;
        preferences.captureBar.dockEdge = defaults.dockEdge;
        preferences.captureBar.screenName = defaults.screenName;
        preferences.captureBar.positionRatio = defaults.positionRatio;
        preferences.captureBar.freePosition = defaults.freePosition;
        preferences.captureBar.hasFreePosition = defaults.hasFreePosition;
        preferences.captureBar.horizontal = defaults.horizontal;
    }
}

void InterfaceSettings::restoreDefaults()
{
    ApplicationPreferences preferences;
    setPreferences(preferences);
}
