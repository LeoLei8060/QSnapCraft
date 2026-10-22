#include "screenshotsettings.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QCheckBox *addCheckOption(QVBoxLayout  *layout,
                          QWidget      *parent,
                          const QString &text,
                          const QString &objectName)
{
    auto *checkBox = new QCheckBox(text, parent);
    checkBox->setObjectName(objectName);
    layout->addWidget(checkBox);
    return checkBox;
}
} // namespace

ScreenshotSettings::ScreenshotSettings(QWidget *parent)
    : QWidget(parent)
    , m_anchorColor(0, 0, 0, 255)
{
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 22, 24, 22);
    pageLayout->setSpacing(14);

    auto *detectionGroup = new QGroupBox(tr("目标检测"), this);
    auto *detectionLayout = new QVBoxLayout(detectionGroup);
    m_detectWindows
        = addCheckOption(detectionLayout, detectionGroup, tr("自动检测窗口"), QStringLiteral("check_DetectWindows"));
    m_detectUiElements = addCheckOption(detectionLayout,
                                        detectionGroup,
                                        tr("自动检测窗口内的 UI 元素"),
                                        QStringLiteral("check_DetectUiElements"));
    m_captureCursor
        = addCheckOption(detectionLayout, detectionGroup, tr("截图包含鼠标光标"), QStringLiteral("check_CaptureCursor"));
    pageLayout->addWidget(detectionGroup);

    auto *selectionGroup = new QGroupBox(tr("选区辅助"), this);
    auto *selectionLayout = new QVBoxLayout(selectionGroup);
    m_showAnchors
        = addCheckOption(selectionLayout, selectionGroup, tr("显示八向控制点"), QStringLiteral("check_ShowAnchors"));
    m_showTips
        = addCheckOption(selectionLayout, selectionGroup, tr("显示快捷键提示"), QStringLiteral("check_ShowTips"));
    m_autoCrossLines
        = addCheckOption(selectionLayout, selectionGroup, tr("自动显示十字线"), QStringLiteral("check_AutoCrossLines"));
    m_hide1x1 = addCheckOption(selectionLayout,
                               selectionGroup,
                               tr("隐藏 1×1 选区边框"),
                               QStringLiteral("check_Hide1x1"));
    m_drawOutsideArea = addCheckOption(selectionLayout,
                                       selectionGroup,
                                       tr("使用外部遮罩突出选区"),
                                       QStringLiteral("check_DrawOutsideArea"));

    auto *appearanceRow = new QWidget(selectionGroup);
    auto *appearanceLayout = new QHBoxLayout(appearanceRow);
    appearanceLayout->setContentsMargins(0, 0, 0, 0);
    appearanceLayout->addWidget(new QLabel(tr("边框宽度："), appearanceRow));
    m_borderWidth = new QSpinBox(appearanceRow);
    m_borderWidth->setObjectName(QStringLiteral("spin_SelectionBorderWidth"));
    m_borderWidth->setRange(1, 30);
    m_borderWidth->setSuffix(tr(" 像素"));
    appearanceLayout->addWidget(m_borderWidth);
    appearanceLayout->addSpacing(16);
    appearanceLayout->addWidget(new QLabel(tr("控制点颜色："), appearanceRow));
    m_anchorColorButton = new QPushButton(appearanceRow);
    m_anchorColorButton->setObjectName(QStringLiteral("btn_AnchorColor"));
    appearanceLayout->addWidget(m_anchorColorButton);
    appearanceLayout->addStretch();
    selectionLayout->addWidget(appearanceRow);
    pageLayout->addWidget(selectionGroup);

    auto *magnifierGroup = new QGroupBox(tr("放大镜"), this);
    auto *magnifierLayout = new QVBoxLayout(magnifierGroup);
    m_showMagnifier
        = addCheckOption(magnifierLayout, magnifierGroup, tr("显示放大镜"), QStringLiteral("check_ShowMagnifier"));
    m_magnifierMask = addCheckOption(magnifierLayout,
                                     magnifierGroup,
                                     tr("显示当前像素遮罩"),
                                     QStringLiteral("check_MagnifierMask"));
    m_magnifierBorder = addCheckOption(magnifierLayout,
                                       magnifierGroup,
                                       tr("显示选区边框"),
                                       QStringLiteral("check_MagnifierBorder"));
    m_magnifierAnchors = addCheckOption(magnifierLayout,
                                        magnifierGroup,
                                        tr("显示控制点"),
                                        QStringLiteral("check_MagnifierAnchors"));
    pageLayout->addWidget(magnifierGroup);

    auto *confirmationGroup = new QGroupBox(tr("确认与退出"), this);
    auto *confirmationLayout = new QVBoxLayout(confirmationGroup);
    m_confirmOnEscape = addCheckOption(confirmationLayout,
                                       confirmationGroup,
                                       tr("按 Esc 取消前询问"),
                                       QStringLiteral("check_ConfirmEscape"));
    m_abortOnFocusOut = addCheckOption(confirmationLayout,
                                       confirmationGroup,
                                       tr("失去焦点时取消截图"),
                                       QStringLiteral("check_AbortOnFocusOut"));
    pageLayout->addWidget(confirmationGroup);

    auto *historyGroup = new QGroupBox(tr("选区历史"), this);
    auto *historyLayout = new QHBoxLayout(historyGroup);
    historyLayout->addWidget(new QLabel(tr("最多记录："), historyGroup));
    m_maxHistoryAreas = new QSpinBox(historyGroup);
    m_maxHistoryAreas->setObjectName(QStringLiteral("spin_MaxHistoryAreas"));
    m_maxHistoryAreas->setRange(1, 100);
    m_maxHistoryAreas->setSuffix(tr(" 个区域"));
    historyLayout->addWidget(m_maxHistoryAreas);
    historyLayout->addSpacing(16);
    m_loopHistory = new QCheckBox(tr("循环回放"), historyGroup);
    m_loopHistory->setObjectName(QStringLiteral("check_LoopHistory"));
    historyLayout->addWidget(m_loopHistory);
    historyLayout->addStretch();
    pageLayout->addWidget(historyGroup);

    auto *gestureGroup = new QGroupBox(tr("输出动作"), this);
    auto *gestureLayout = new QGridLayout(gestureGroup);
    gestureLayout->setColumnStretch(1, 1);
    m_enterAction = createOutputActionCombo(QStringLiteral("combo_EnterAction"));
    m_doubleClickAction = createOutputActionCombo(QStringLiteral("combo_DoubleClickAction"));
    m_middleClickAction = createOutputActionCombo(QStringLiteral("combo_MiddleClickAction"));
    m_enterExit = new QCheckBox(tr("执行后退出截图"), gestureGroup);
    m_enterExit->setObjectName(QStringLiteral("check_EnterExit"));
    m_doubleClickExit = new QCheckBox(tr("执行后退出截图"), gestureGroup);
    m_doubleClickExit->setObjectName(QStringLiteral("check_DoubleClickExit"));
    m_middleClickExit = new QCheckBox(tr("执行后退出截图"), gestureGroup);
    m_middleClickExit->setObjectName(QStringLiteral("check_MiddleClickExit"));

    const QStringList gestureLabels = {tr("回车键："), tr("双击："), tr("中键单击：")};
    QComboBox *const    gestureCombos[] = {m_enterAction, m_doubleClickAction, m_middleClickAction};
    QCheckBox *const    gestureChecks[] = {m_enterExit, m_doubleClickExit, m_middleClickExit};
    for (int index = 0; index < 3; ++index) {
        gestureLayout->addWidget(new QLabel(gestureLabels.at(index), gestureGroup), index, 0);
        gestureLayout->addWidget(gestureCombos[index], index, 1);
        gestureLayout->addWidget(gestureChecks[index], index, 2);
        connect(gestureCombos[index],
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                &ScreenshotSettings::sigChanged);
        connect(gestureChecks[index], &QCheckBox::toggled, this, &ScreenshotSettings::sigChanged);
    }
    pageLayout->addWidget(gestureGroup);
    pageLayout->addStretch();

    QCheckBox *const checks[] = {m_detectWindows,
                                 m_detectUiElements,
                                 m_showAnchors,
                                 m_showTips,
                                 m_autoCrossLines,
                                 m_captureCursor,
                                 m_showMagnifier,
                                 m_magnifierMask,
                                 m_magnifierBorder,
                                 m_magnifierAnchors,
                                 m_confirmOnEscape,
                                 m_abortOnFocusOut,
                                 m_hide1x1,
                                 m_drawOutsideArea,
                                 m_loopHistory};
    for (QCheckBox *checkBox : checks) {
        connect(checkBox, &QCheckBox::toggled, this, &ScreenshotSettings::sigChanged);
    }
    connect(m_borderWidth, &QSpinBox::valueChanged, this, &ScreenshotSettings::sigChanged);
    connect(m_maxHistoryAreas, &QSpinBox::valueChanged, this, &ScreenshotSettings::sigChanged);
    connect(m_anchorColorButton, &QPushButton::clicked, this, &ScreenshotSettings::onChooseAnchorColor);
    connect(m_showMagnifier, &QCheckBox::toggled, this, &ScreenshotSettings::updateMagnifierState);

    updateColorButton();
    updateMagnifierState();
}

ScreenshotSettings::~ScreenshotSettings() = default;

void ScreenshotSettings::setPreferences(const ApplicationPreferences &preferences)
{
    const auto &options = preferences.capture;

    const QSignalBlocker detectWindowsBlocker(m_detectWindows);
    const QSignalBlocker detectUiBlocker(m_detectUiElements);
    const QSignalBlocker anchorsBlocker(m_showAnchors);
    const QSignalBlocker tipsBlocker(m_showTips);
    const QSignalBlocker crossLinesBlocker(m_autoCrossLines);
    const QSignalBlocker hide1x1Blocker(m_hide1x1);
    const QSignalBlocker outsideBlocker(m_drawOutsideArea);
    const QSignalBlocker historyBlocker(m_loopHistory);
    const QSignalBlocker cursorBlocker(m_captureCursor);
    const QSignalBlocker magnifierBlocker(m_showMagnifier);
    const QSignalBlocker magnifierMaskBlocker(m_magnifierMask);
    const QSignalBlocker magnifierBorderBlocker(m_magnifierBorder);
    const QSignalBlocker magnifierAnchorsBlocker(m_magnifierAnchors);
    const QSignalBlocker confirmBlocker(m_confirmOnEscape);
    const QSignalBlocker focusBlocker(m_abortOnFocusOut);
    const QSignalBlocker maxHistoryBlocker(m_maxHistoryAreas);
    const QSignalBlocker borderBlocker(m_borderWidth);

    m_detectWindows->setChecked(options.detectWindows);
    m_detectUiElements->setChecked(options.detectUiElements);
    m_showAnchors->setChecked(options.showAnchors);
    m_showTips->setChecked(options.showTips);
    m_autoCrossLines->setChecked(options.autoCrossLines);
    m_hide1x1->setChecked(options.hide1x1);
    m_drawOutsideArea->setChecked(options.drawOutsideArea);
    m_loopHistory->setChecked(options.loopHistoryAreas);
    m_captureCursor->setChecked(options.captureCursor);
    m_showMagnifier->setChecked(options.showMagnifier);
    m_magnifierMask->setChecked(options.showMaskInMagnifier);
    m_magnifierBorder->setChecked(options.showBorderInMagnifier);
    m_magnifierAnchors->setChecked(options.showAnchorsInMagnifier);
    m_confirmOnEscape->setChecked(options.askForConfirmOnEscape);
    m_abortOnFocusOut->setChecked(options.abortOnFocusOut);
    m_maxHistoryAreas->setValue(options.maxHistoryAreas);
    m_borderWidth->setValue(options.borderWidth);
    m_anchorColor = options.anchorBorderColor;

    setOutputGesture(m_enterAction,
                     m_enterExit,
                     options.enterKeyOutput,
                     options.enterAction,
                     options.enterExit);
    setOutputGesture(m_doubleClickAction,
                     m_doubleClickExit,
                     options.doubleClickOutput,
                     options.doubleClickAction,
                     options.doubleClickExit);
    setOutputGesture(m_middleClickAction,
                     m_middleClickExit,
                     options.middleClickOutput,
                     options.middleClickAction,
                     options.middleClickExit);
    updateColorButton();
    updateMagnifierState();
}

void ScreenshotSettings::writePreferences(ApplicationPreferences &preferences) const
{
    auto &options = preferences.capture;

    options.detectWindows = m_detectWindows->isChecked();
    options.detectUiElements = m_detectUiElements->isChecked();
    options.showAnchors = m_showAnchors->isChecked();
    options.showTips = m_showTips->isChecked();
    options.autoCrossLines = m_autoCrossLines->isChecked();
    options.hide1x1 = m_hide1x1->isChecked();
    options.drawOutsideArea = m_drawOutsideArea->isChecked();
    options.loopHistoryAreas = m_loopHistory->isChecked();
    options.captureCursor = m_captureCursor->isChecked();
    options.showMagnifier = m_showMagnifier->isChecked();
    options.showMaskInMagnifier = m_magnifierMask->isChecked();
    options.showBorderInMagnifier = m_magnifierBorder->isChecked();
    options.showAnchorsInMagnifier = m_magnifierAnchors->isChecked();
    options.askForConfirmOnEscape = m_confirmOnEscape->isChecked();
    options.abortOnFocusOut = m_abortOnFocusOut->isChecked();
    options.maxHistoryAreas = m_maxHistoryAreas->value();
    options.borderWidth = m_borderWidth->value();
    options.anchorBorderColor = m_anchorColor;

    options.enterKeyOutput = m_enterAction->currentData().toInt() != static_cast<int>(CaptureOutputAction::kNone);
    options.enterAction = static_cast<CaptureOutputAction>(m_enterAction->currentData().toInt());
    options.enterExit = m_enterExit->isChecked();
    options.doubleClickOutput
        = m_doubleClickAction->currentData().toInt() != static_cast<int>(CaptureOutputAction::kNone);
    options.doubleClickAction = static_cast<CaptureOutputAction>(m_doubleClickAction->currentData().toInt());
    options.doubleClickExit = m_doubleClickExit->isChecked();
    options.middleClickOutput
        = m_middleClickAction->currentData().toInt() != static_cast<int>(CaptureOutputAction::kNone);
    options.middleClickAction = static_cast<CaptureOutputAction>(m_middleClickAction->currentData().toInt());
    options.middleClickExit = m_middleClickExit->isChecked();
}

void ScreenshotSettings::restoreDefaults()
{
    ApplicationPreferences preferences;
    setPreferences(preferences);
}

QComboBox *ScreenshotSettings::createOutputActionCombo(const QString &objectName)
{
    auto *combo = new QComboBox(this);
    combo->setObjectName(objectName);
    combo->addItem(tr("无"), static_cast<int>(CaptureOutputAction::kNone));
    combo->addItem(tr("复制到剪贴板"), static_cast<int>(CaptureOutputAction::kCopy));
    combo->addItem(tr("贴到屏幕"), static_cast<int>(CaptureOutputAction::kPin));
    combo->addItem(tr("保存到文件"), static_cast<int>(CaptureOutputAction::kSave));
    combo->addItem(tr("快捷保存"), static_cast<int>(CaptureOutputAction::kQuickSave));
    return combo;
}

void ScreenshotSettings::setOutputGesture(QComboBox          *actionCombo,
                                          QCheckBox          *exitCheck,
                                          bool                enabled,
                                          CaptureOutputAction action,
                                          bool                exit)
{
    const QSignalBlocker actionBlocker(actionCombo);
    const QSignalBlocker exitBlocker(exitCheck);
    const int           action_index = enabled ? actionCombo->findData(static_cast<int>(action)) : 0;
    actionCombo->setCurrentIndex(action_index >= 0 ? action_index : 0);
    exitCheck->setChecked(exit);
}

void ScreenshotSettings::onChooseAnchorColor()
{
    const QColor selected = QColorDialog::getColor(m_anchorColor, this, tr("选择控制点颜色"));
    if (!selected.isValid()) {
        return;
    }
    m_anchorColor = selected;
    updateColorButton();
    emit sigChanged();
}

void ScreenshotSettings::updateMagnifierState()
{
    const bool enabled = m_showMagnifier->isChecked();
    m_magnifierMask->setEnabled(enabled);
    m_magnifierBorder->setEnabled(enabled);
    m_magnifierAnchors->setEnabled(enabled);
}

void ScreenshotSettings::updateColorButton()
{
    m_anchorColorButton->setText(m_anchorColor.name(QColor::HexArgb));
    m_anchorColorButton->setStyleSheet(
        QStringLiteral("QPushButton { text-align: left; padding-left: 12px;"
                       " border-left: 18px solid %1; }")
            .arg(m_anchorColor.name(QColor::HexRgb)));
}
