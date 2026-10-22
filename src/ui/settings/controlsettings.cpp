#include "controlsettings.h"

#include "ui/widgets/qhotkeylineedit.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
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
    formLayout->addRow(tr("自动捕获："), m_captureHotkey);
    formLayout->addRow(tr("自动捕获并复制："), m_captureCopyHotkey);
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
            emit sigChanged();
        });
        connect(lineEdit, &QHotKeyLineEdit::sigClearKey, this, &ControlSettings::sigChanged);
    };
    connectHotkey(m_captureHotkey);
    connectHotkey(m_captureCopyHotkey);
}

ControlSettings::~ControlSettings() = default;

void ControlSettings::setPreferences(const ApplicationPreferences &preferences)
{
    m_captureHotkey->setShortcut(preferences.control.captureHotkey);
    m_captureCopyHotkey->setShortcut(preferences.control.captureCopyHotkey);
}

void ControlSettings::writePreferences(ApplicationPreferences &preferences) const
{
    preferences.control.captureHotkey = m_captureHotkey->shortcut();
    preferences.control.captureCopyHotkey = m_captureCopyHotkey->shortcut();
}

void ControlSettings::restoreDefaults()
{
    const ControlSettingsData defaults;
    m_captureHotkey->setShortcut(defaults.captureHotkey);
    m_captureCopyHotkey->setShortcut(defaults.captureCopyHotkey);
}
