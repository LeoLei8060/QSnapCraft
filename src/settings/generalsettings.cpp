#include "generalsettings.h"
#include "ui_general.h"
#include "globalconfig.h"
#include "utils/systemutils.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMessageBox>
#include <QCoreApplication>

GeneralSettings::GeneralSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GeneralSettings)
{
    ui->setupUi(this);

    // 连接信号槽
    connect(ui->checkBox, &QCheckBox::toggled, this, &GeneralSettings::onAutoStartChanged);
    connect(ui->btn_OpenFolders, &QPushButton::clicked, this, &GeneralSettings::onOpenFolderClicked);
    connect(ui->btn_OpenFile, &QPushButton::clicked, this, &GeneralSettings::onOpenFileClicked);
    connect(GlobalConfig::instance(), &GlobalConfig::configChanged, this, &GeneralSettings::onConfigChanged);

    // 禁用配置文件路径修改功能
    ui->btn_Change->setVisible(false);

    loadSettings();
}

GeneralSettings::~GeneralSettings()
{
    delete ui;
}

void GeneralSettings::loadSettings()
{
    const auto &data = GlobalConfig::instance()->generalData();
    ui->checkBox->setChecked(data.autoStart);
    updateConfigPath(data.savePath);
}

void GeneralSettings::updateConfigPath(const QString &path)
{
    ui->lineEdit->setText(QDir::toNativeSeparators(path));
}

void GeneralSettings::onAutoStartChanged(bool checked)
{
    auto data = GlobalConfig::instance()->generalData();
    data.autoStart = checked;
    GlobalConfig::instance()->setGeneralData(data);

    // 设置开机自启动
    SystemUtils::setAutoStart(checked);
}

void GeneralSettings::onOpenFolderClicked()
{
    const auto &data = GlobalConfig::instance()->generalData();
    QString folderPath = QFileInfo(data.savePath).absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

void GeneralSettings::onOpenFileClicked()
{
    const auto &data = GlobalConfig::instance()->generalData();
    QDesktopServices::openUrl(QUrl::fromLocalFile(data.savePath));
}

void GeneralSettings::onConfigChanged()
{
    loadSettings();
}
