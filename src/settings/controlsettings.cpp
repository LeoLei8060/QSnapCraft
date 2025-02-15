#include "controlsettings.h"
#include "globalconfig.h"
#include "ui_control.h"
#include "utils/shortcutmanager.h"
#include <QKeyEvent>
#include <QMessageBox>
#include <qhotkeylineedit.h>

ControlSettings::ControlSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ControlSettings)
{
    ui->setupUi(this);

    initializeConnect();

    loadSettings();
    ui->groupBox_2->setDisabled(true);
}

ControlSettings::~ControlSettings()
{
    delete ui;
}

void ControlSettings::initializeConnect()
{
    // 创建快捷键输入框和对应ID的映射
    static const QMap<QHotKeyLineEdit *, int> hotkeyMap
        = {{ui->lineEdit, ShortcutManager::captureKey},
           {ui->lineEdit_2, ShortcutManager::captureCopyKey},
           {ui->lineEdit_3, ShortcutManager::showLastPinKey},
           {ui->lineEdit_4, ShortcutManager::showPinsKey}};

    // 为每个快捷键输入框连接信号
    for (auto it = hotkeyMap.begin(); it != hotkeyMap.end(); ++it) {
        connect(it.key(), &QHotKeyLineEdit::sigClearKey, this, [this, id = it.value()]() {
            ShortcutManager::instance()->unregisterHotkey(id);
            updateData(id, "");
        });

        connect(it.key(),
                &QHotKeyLineEdit::sigKeyPressed,
                this,
                [this, lineEdit = it.key(), id = it.value()](const QString &key) {
                    bool state = ShortcutManager::instance()->registerHotkey(key, id);
                    lineEdit->setText(key, state);
                    updateData(id, key);
                });
    }

    // 连接恢复默认按钮
    connect(ui->btn_RestoreDef,
            &QPushButton::clicked,
            this,
            &ControlSettings::onRestoreDefaultClicked);
}

void ControlSettings::onRestoreDefaultClicked()
{
    ControlSettingsData data;
    GlobalConfig::instance()->setControlData(data);
    ui->lineEdit->resetText(data.captureHotkey);
    ui->lineEdit_2->resetText(data.captureCopyHotkey);
    ui->lineEdit_3->resetText(data.showLastPinHotkey);
    ui->lineEdit_4->resetText(data.showPinsHotkey);
}

void ControlSettings::loadSettings()
{
    const auto &data = GlobalConfig::instance()->controlData();

    // 更新快捷键显示
    bool state = ShortcutManager::instance()->isHotkeyRegistered(data.captureHotkey,
                                                                 ShortcutManager::captureKey);
    ui->lineEdit->setText(data.captureHotkey, state);

    state = ShortcutManager::instance()->isHotkeyRegistered(data.captureCopyHotkey,
                                                            ShortcutManager::captureCopyKey);
    ui->lineEdit_2->setText(data.captureCopyHotkey, state);

    state = ShortcutManager::instance()->isHotkeyRegistered(data.showLastPinHotkey,
                                                            ShortcutManager::showLastPinKey);
    ui->lineEdit_3->setText(data.showLastPinHotkey, state);

    state = ShortcutManager::instance()->isHotkeyRegistered(data.showPinsHotkey,
                                                            ShortcutManager::showPinsKey);
    ui->lineEdit_4->setText(data.showPinsHotkey, state);
}

QString ControlSettings::getDefaultShortcutKey(int type) const
{
    switch (type) {
    case 0:
        return "F1"; // 截屏
    case 1:
        return "Ctrl+F1"; // 截屏并复制
    case 2:
        return "F3"; // 显示最近贴图
    case 3:
        return "Shift+F3"; // 隐藏/显示所有贴图
    default:
        return QString();
    }
}

void ControlSettings::updateShortcutDisplay(QHotKeyLineEdit *lineEdit, const QString &key) {}

QString ControlSettings::keyEventToString(QKeyEvent *event) const
{
    int key = event->key();
    if (key == Qt::Key_unknown) {
        return QString();
    }

    QStringList           sequence;
    Qt::KeyboardModifiers modifiers = event->modifiers();

    // 添加修饰键
    if (modifiers & Qt::ControlModifier) {
        sequence << "Ctrl";
    }
    if (modifiers & Qt::ShiftModifier) {
        sequence << "Shift";
    }
    if (modifiers & Qt::AltModifier) {
        sequence << "Alt";
    }
    if (modifiers & Qt::MetaModifier) {
        sequence << "Win";
    }

    // 添加主键
    if (key != Qt::Key_Control && key != Qt::Key_Shift && key != Qt::Key_Alt
        && key != Qt::Key_Meta) {
        sequence << QKeySequence(key).toString();
    }

    return sequence.join("+");
}

void ControlSettings::updateData(int id, const QString &val)
{
    auto &controlData = GlobalConfig::instance()->controlData();
    if (id == ShortcutManager::captureKey)
        controlData.captureHotkey = val;
    else if (id == ShortcutManager::captureCopyKey)
        controlData.captureCopyHotkey = val;
    else if (id == ShortcutManager::showLastPinKey)
        controlData.showLastPinHotkey = val;
    else if (id == ShortcutManager::showPinsKey)
        controlData.showPinsHotkey = val;
    GlobalConfig::instance()->setControlData(controlData);
}

QStringList ControlSettings::parseShortcutString(const QString &shortcut)
{
    // 按+号分割字符串，并去除每个部分的空白
    QStringList keys = shortcut.split('+', Qt::SkipEmptyParts);
    for (QString &key : keys) {
        key = key.trimmed();
    }
    return keys;
}

UINT ControlSettings::getVirtualKeyCode(const QString &keyStr)
{
    // 特殊键映射
    static const QMap<QString, UINT> specialKeys = {{"Ctrl", VK_CONTROL},
                                                    {"Shift", VK_SHIFT},
                                                    {"Alt", VK_MENU},
                                                    {"Win", VK_LWIN},
                                                    {"Tab", VK_TAB},
                                                    {"Esc", VK_ESCAPE},
                                                    {"Space", VK_SPACE},
                                                    {"Enter", VK_RETURN},
                                                    {"Backspace", VK_BACK},
                                                    {"Insert", VK_INSERT},
                                                    {"Delete", VK_DELETE},
                                                    {"Home", VK_HOME},
                                                    {"End", VK_END},
                                                    {"PgUp", VK_PRIOR},
                                                    {"PgDown", VK_NEXT},
                                                    {"Left", VK_LEFT},
                                                    {"Right", VK_RIGHT},
                                                    {"Up", VK_UP},
                                                    {"Down", VK_DOWN}};

    // 检查是否是特殊键
    if (specialKeys.contains(keyStr)) {
        return specialKeys[keyStr];
    }

    // 检查功能键 (F1-F24)
    if (keyStr.startsWith('F')) {
        bool ok;
        int  num = keyStr.midRef(1).toInt(&ok);
        if (ok && num >= 1 && num <= 24) {
            return VK_F1 + num - 1;
        }
    }

    // 检查数字键
    if (keyStr.length() == 1) {
        QChar ch = keyStr[0];
        if (ch.isDigit()) {
            return '0' + ch.digitValue();
        }
        // 检查字母键
        if (ch.isLetter()) {
            return ch.toUpper().toLatin1();
        }
    }

    // 无法识别的键
    return 0;
}
