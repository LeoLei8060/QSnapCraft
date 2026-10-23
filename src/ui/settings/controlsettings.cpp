#include "controlsettings.h"

#include "platform/windows/shortcutmanager.h"
#include "ui/widgets/qhotkeylineedit.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

ControlSettings::ControlSettings(QWidget *parent)
    : QWidget(parent)
{
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 22, 24, 22);
    pageLayout->setSpacing(16);

    auto *hotkeyGroup = new QGroupBox(tr("全局快捷键"), this);
    auto *formLayout = new QFormLayout(hotkeyGroup);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_captureHotkey = new QHotKeyLineEdit(hotkeyGroup);
    m_captureHotkey->setObjectName(QStringLiteral("hotkey_Capture"));
    m_captureCopyHotkey = new QHotKeyLineEdit(hotkeyGroup);
    m_captureCopyHotkey->setObjectName(QStringLiteral("hotkey_CaptureCopy"));
    m_captureFullDesktopHotkey = new QHotKeyLineEdit(hotkeyGroup);
    m_captureFullDesktopHotkey->setObjectName(QStringLiteral("hotkey_CaptureFullDesktop"));
    m_captureLastAreaHotkey = new QHotKeyLineEdit(hotkeyGroup);
    m_captureLastAreaHotkey->setObjectName(QStringLiteral("hotkey_CaptureLastArea"));
    m_captureActiveScreenHotkey = new QHotKeyLineEdit(hotkeyGroup);
    m_captureActiveScreenHotkey->setObjectName(QStringLiteral("hotkey_CaptureActiveScreen"));
    m_captureFixedSizeHotkey = new QHotKeyLineEdit(hotkeyGroup);
    m_captureFixedSizeHotkey->setObjectName(QStringLiteral("hotkey_CaptureFixedSize"));
    m_clickThroughHotkey = new QHotKeyLineEdit(hotkeyGroup);
    m_clickThroughHotkey->setObjectName(QStringLiteral("hotkey_ClickThrough"));
    formLayout->addRow(tr("自动捕获："),
                       createHotkeyRow(m_captureHotkey, &m_captureStatus, QStringLiteral("label_HotkeyCaptureStatus")));
    formLayout->addRow(tr("自动捕获并复制："),
                       createHotkeyRow(m_captureCopyHotkey,
                                       &m_captureCopyStatus,
                                       QStringLiteral("label_HotkeyCaptureCopyStatus")));
    formLayout->addRow(tr("全桌面截图："),
                       createHotkeyRow(m_captureFullDesktopHotkey,
                                       &m_captureFullDesktopStatus,
                                       QStringLiteral("label_HotkeyCaptureFullDesktopStatus")));
    formLayout->addRow(tr("上次区域截图："),
                       createHotkeyRow(m_captureLastAreaHotkey,
                                       &m_captureLastAreaStatus,
                                       QStringLiteral("label_HotkeyCaptureLastAreaStatus")));
    formLayout->addRow(tr("当前屏幕截图："),
                       createHotkeyRow(m_captureActiveScreenHotkey,
                                       &m_captureActiveScreenStatus,
                                       QStringLiteral("label_HotkeyCaptureActiveScreenStatus")));
    formLayout->addRow(tr("固定尺寸居中截图："),
                       createHotkeyRow(m_captureFixedSizeHotkey,
                                       &m_captureFixedSizeStatus,
                                       QStringLiteral("label_HotkeyCaptureFixedSizeStatus")));
    formLayout->addRow(tr("切换贴图穿透："),
                       createHotkeyRow(m_clickThroughHotkey,
                                       &m_clickThroughStatus,
                                       QStringLiteral("label_HotkeyClickThroughStatus")));

    auto *fixed_size_row = new QWidget(hotkeyGroup);
    auto *fixed_size_layout = new QHBoxLayout(fixed_size_row);
    fixed_size_layout->setContentsMargins(0, 0, 0, 0);
    fixed_size_layout->setSpacing(6);
    m_fixedWidth = new QSpinBox(fixed_size_row);
    m_fixedWidth->setObjectName(QStringLiteral("spin_FixedCaptureWidth"));
    m_fixedWidth->setRange(50, 16384);
    m_fixedWidth->setSuffix(tr(" px"));
    m_fixedHeight = new QSpinBox(fixed_size_row);
    m_fixedHeight->setObjectName(QStringLiteral("spin_FixedCaptureHeight"));
    m_fixedHeight->setRange(50, 16384);
    m_fixedHeight->setSuffix(tr(" px"));
    fixed_size_layout->addWidget(m_fixedWidth);
    fixed_size_layout->addWidget(new QLabel(QStringLiteral("×"), fixed_size_row));
    fixed_size_layout->addWidget(m_fixedHeight);
    fixed_size_layout->addStretch();
    formLayout->addRow(tr("固定尺寸："), fixed_size_row);
    pageLayout->addWidget(hotkeyGroup);

    auto *hint = new QLabel(tr("点击输入框后按下新的组合键。修改将在点击应用后重新注册。"), this);
    hint->setWordWrap(true);
    pageLayout->addWidget(hint);

    auto *builtInGroup = new QGroupBox(tr("截图内置快捷键"), this);
    auto *builtInLayout = new QFormLayout(builtInGroup);
    builtInLayout->addRow(tr("复制选区："), new QLabel(QStringLiteral("Ctrl+C"), builtInGroup));
    builtInLayout->addRow(tr("保存："), new QLabel(QStringLiteral("Ctrl+S"), builtInGroup));
    builtInLayout->addRow(tr("快捷保存："), new QLabel(QStringLiteral("Ctrl+Shift+S"), builtInGroup));
    builtInLayout->addRow(tr("贴到屏幕："), new QLabel(QStringLiteral("Ctrl+T"), builtInGroup));
    builtInLayout->addRow(tr("取消截图："), new QLabel(QStringLiteral("Esc"), builtInGroup));
    pageLayout->addWidget(builtInGroup);
    pageLayout->addStretch();

    const auto connectHotkey = [this](QHotKeyLineEdit *lineEdit) {
        connect(lineEdit, &QHotKeyLineEdit::sigKeyPressed, this, [this, lineEdit](const QString &key) {
            lineEdit->setText(key, true);
            refreshHotkeyStatuses();
            emit sigChanged();
        });
        connect(lineEdit, &QHotKeyLineEdit::sigClearKey, this, [this]() {
            refreshHotkeyStatuses();
            emit sigChanged();
        });
    };
    connectHotkey(m_captureHotkey);
    connectHotkey(m_captureCopyHotkey);
    connectHotkey(m_captureFullDesktopHotkey);
    connectHotkey(m_captureLastAreaHotkey);
    connectHotkey(m_captureActiveScreenHotkey);
    connectHotkey(m_captureFixedSizeHotkey);
    connectHotkey(m_clickThroughHotkey);
    connect(m_fixedWidth, &QSpinBox::valueChanged, this, &ControlSettings::sigChanged);
    connect(m_fixedHeight, &QSpinBox::valueChanged, this, &ControlSettings::sigChanged);
}

ControlSettings::~ControlSettings() = default;

void ControlSettings::setPreferences(const ApplicationPreferences &preferences)
{
    m_captureHotkey->setShortcut(preferences.control.captureHotkey);
    m_captureCopyHotkey->setShortcut(preferences.control.captureCopyHotkey);
    m_captureFullDesktopHotkey->setShortcut(preferences.control.captureFullDesktopHotkey);
    m_captureLastAreaHotkey->setShortcut(preferences.control.captureLastAreaHotkey);
    m_captureActiveScreenHotkey->setShortcut(preferences.control.captureActiveScreenHotkey);
    m_captureFixedSizeHotkey->setShortcut(preferences.control.captureFixedSizeHotkey);
    m_clickThroughHotkey->setShortcut(preferences.control.clickThroughHotkey);
    m_fixedWidth->setValue(preferences.control.fixedCaptureWidth);
    m_fixedHeight->setValue(preferences.control.fixedCaptureHeight);
    refreshHotkeyStatuses();
}

void ControlSettings::writePreferences(ApplicationPreferences &preferences) const
{
    preferences.control.captureHotkey = m_captureHotkey->shortcut();
    preferences.control.captureCopyHotkey = m_captureCopyHotkey->shortcut();
    preferences.control.captureFullDesktopHotkey = m_captureFullDesktopHotkey->shortcut();
    preferences.control.captureLastAreaHotkey = m_captureLastAreaHotkey->shortcut();
    preferences.control.captureActiveScreenHotkey = m_captureActiveScreenHotkey->shortcut();
    preferences.control.captureFixedSizeHotkey = m_captureFixedSizeHotkey->shortcut();
    preferences.control.clickThroughHotkey = m_clickThroughHotkey->shortcut();
    preferences.control.fixedCaptureWidth = m_fixedWidth->value();
    preferences.control.fixedCaptureHeight = m_fixedHeight->value();
}

void ControlSettings::restoreDefaults()
{
    const ControlSettingsData defaults;
    m_captureHotkey->setShortcut(defaults.captureHotkey);
    m_captureCopyHotkey->setShortcut(defaults.captureCopyHotkey);
    m_captureFullDesktopHotkey->setShortcut(defaults.captureFullDesktopHotkey);
    m_captureLastAreaHotkey->setShortcut(defaults.captureLastAreaHotkey);
    m_captureActiveScreenHotkey->setShortcut(defaults.captureActiveScreenHotkey);
    m_captureFixedSizeHotkey->setShortcut(defaults.captureFixedSizeHotkey);
    m_clickThroughHotkey->setShortcut(defaults.clickThroughHotkey);
    m_fixedWidth->setValue(defaults.fixedCaptureWidth);
    m_fixedHeight->setValue(defaults.fixedCaptureHeight);
    refreshHotkeyStatuses();
}

QWidget *ControlSettings::createHotkeyRow(QHotKeyLineEdit *lineEdit, QLabel **status, const QString &statusObjectName)
{
    auto *row = new QWidget(this);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(lineEdit, 1);

    *status = new QLabel(row);
    (*status)->setObjectName(statusObjectName);
    (*status)->setWordWrap(true);
    layout->addWidget(*status);
    return row;
}

void ControlSettings::refreshHotkeyStatuses()
{
    const auto update_status = [](QLabel *label, const QString &shortcut, ShortcutManager::EKeyType key) {
        const QString error = ShortcutManager::instance()->hotkeyError(key);
        if (!error.isEmpty()) {
            label->setText(error);
            label->setStyleSheet(QStringLiteral("color: #FF7B72;"));
        } else if (ShortcutManager::instance()->isHotkeyRegistered(shortcut, key)) {
            label->setText(QStringLiteral("已注册"));
            label->setStyleSheet(QStringLiteral("color: #7EE787;"));
        } else {
            label->setText(QStringLiteral("尚未注册"));
            label->setStyleSheet(QStringLiteral("color: #A0A0A0;"));
        }
    };

    update_status(m_captureStatus, m_captureHotkey->shortcut(), ShortcutManager::captureKey);
    update_status(m_captureCopyStatus, m_captureCopyHotkey->shortcut(), ShortcutManager::captureCopyKey);
    update_status(m_captureFullDesktopStatus,
                  m_captureFullDesktopHotkey->shortcut(),
                  ShortcutManager::captureFullDesktopKey);
    update_status(m_captureLastAreaStatus, m_captureLastAreaHotkey->shortcut(), ShortcutManager::captureLastAreaKey);
    update_status(m_captureActiveScreenStatus,
                  m_captureActiveScreenHotkey->shortcut(),
                  ShortcutManager::captureActiveScreenKey);
    update_status(m_captureFixedSizeStatus, m_captureFixedSizeHotkey->shortcut(), ShortcutManager::captureFixedSizeKey);
    update_status(m_clickThroughStatus, m_clickThroughHotkey->shortcut(), ShortcutManager::clickThroughKey);
}
