#include "screenshotsettings.h"
#include "ui_screenshot.h"
#include "globalconfig.h"

ScreenshotSettings::ScreenshotSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScreenshotSettings)
{
    ui->setupUi(this);

    // 初始化下拉框选项
    updateComboBoxes();

    // 连接信号槽
    connect(ui->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &ScreenshotSettings::onBorderWidthChanged);
    connect(ui->combox_LeftBtrn, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScreenshotSettings::onLeftButtonActionChanged);
    connect(ui->combox_MiddleBtn, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScreenshotSettings::onMiddleButtonActionChanged);
    connect(ui->combox_Return, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScreenshotSettings::onReturnActionChanged);
    connect(ui->checkBox_LeftBtn, &QCheckBox::toggled,
            this, &ScreenshotSettings::onLeftButtonExitChanged);
    connect(ui->checkBox_MiddleBtn, &QCheckBox::toggled,
            this, &ScreenshotSettings::onMiddleButtonExitChanged);
    connect(ui->checkBox_Return, &QCheckBox::toggled,
            this, &ScreenshotSettings::onReturnExitChanged);
    connect(ui->btn_Restore, &QPushButton::clicked,
            this, &ScreenshotSettings::onRestoreDefaultClicked);
    connect(GlobalConfig::instance(), &GlobalConfig::configChanged,
            this, &ScreenshotSettings::onConfigChanged);

    loadSettings();
}

ScreenshotSettings::~ScreenshotSettings()
{
    delete ui;
}

void ScreenshotSettings::loadSettings()
{
    const auto &data = GlobalConfig::instance()->screenshotData();

    // 设置边框宽度
    ui->spinBox->setValue(data.borderWidth);

    // 设置动作和退出选项
    ui->combox_LeftBtrn->setCurrentIndex(data.doubleLeftAction);
    ui->checkBox_LeftBtn->setChecked(data.bExit_doubleLeft);

    ui->combox_MiddleBtn->setCurrentIndex(data.doubleMiddleAction);
    ui->checkBox_MiddleBtn->setChecked(data.bExit_doubleMiddle);

    ui->combox_Return->setCurrentIndex(data.enterAction);
    ui->checkBox_Return->setChecked(data.bExit_enter);
}

void ScreenshotSettings::updateComboBoxes()
{
    // 清空并重新添加选项
    QStringList actions;
    actions << tr("无")
            << tr("复制到剪贴板")
            << tr("贴到屏幕")
            << tr("保存到文件")
            << tr("快捷保存");

    ui->combox_LeftBtrn->clear();
    ui->combox_MiddleBtn->clear();
    ui->combox_Return->clear();

    ui->combox_LeftBtrn->addItems(actions);
    ui->combox_MiddleBtn->addItems(actions);
    ui->combox_Return->addItems(actions);
}

void ScreenshotSettings::onBorderWidthChanged(int value)
{
    auto data = GlobalConfig::instance()->screenshotData();
    data.borderWidth = value;
    GlobalConfig::instance()->setScreenshotData(data);
}

void ScreenshotSettings::onLeftButtonActionChanged(int index)
{
    auto data = GlobalConfig::instance()->screenshotData();
    data.doubleLeftAction = index;
    GlobalConfig::instance()->setScreenshotData(data);
}

void ScreenshotSettings::onMiddleButtonActionChanged(int index)
{
    auto data = GlobalConfig::instance()->screenshotData();
    data.doubleMiddleAction = index;
    GlobalConfig::instance()->setScreenshotData(data);
}

void ScreenshotSettings::onReturnActionChanged(int index)
{
    auto data = GlobalConfig::instance()->screenshotData();
    data.enterAction = index;
    GlobalConfig::instance()->setScreenshotData(data);
}

void ScreenshotSettings::onLeftButtonExitChanged(bool checked)
{
    auto data = GlobalConfig::instance()->screenshotData();
    data.bExit_doubleLeft = checked;
    GlobalConfig::instance()->setScreenshotData(data);
}

void ScreenshotSettings::onMiddleButtonExitChanged(bool checked)
{
    auto data = GlobalConfig::instance()->screenshotData();
    data.bExit_doubleMiddle = checked;
    GlobalConfig::instance()->setScreenshotData(data);
}

void ScreenshotSettings::onReturnExitChanged(bool checked)
{
    auto data = GlobalConfig::instance()->screenshotData();
    data.bExit_enter = checked;
    GlobalConfig::instance()->setScreenshotData(data);
}

void ScreenshotSettings::onRestoreDefaultClicked()
{
    // 创建默认设置
    ScreenshotSettingsData defaultData;
    defaultData.borderWidth = 1;
    defaultData.doubleLeftAction = ScreenshotSettingsData::SS_CopytoClipboard;
    defaultData.bExit_doubleLeft = true;
    defaultData.doubleMiddleAction = ScreenshotSettingsData::SS_None;
    defaultData.bExit_doubleMiddle = false;
    defaultData.enterAction = ScreenshotSettingsData::SS_CopytoClipboard;
    defaultData.bExit_enter = true;

    // 应用默认设置
    GlobalConfig::instance()->setScreenshotData(defaultData);
}

void ScreenshotSettings::onConfigChanged()
{
    loadSettings();
}
