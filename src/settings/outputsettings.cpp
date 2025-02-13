#include "outputsettings.h"
#include "globalconfig.h"
#include "ui_output.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QUrl>

OutputSettings::OutputSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OutputSettings)
{
    ui->setupUi(this);

    // 启用窗口
    setEnabled(true);

    // 连接信号槽
    connect(ui->lineEdit_FileName,
            &QLineEdit::textChanged,
            this,
            &OutputSettings::onManualFileNameChanged);
    connect(ui->lineEdit_Path,
            &QLineEdit::textChanged,
            this,
            &OutputSettings::onQuickSavePathChanged);
    connect(ui->btn_OpenFolders, &QPushButton::clicked, this, &OutputSettings::onOpenFoldersClicked);
    connect(ui->btn_ChangeFolders,
            &QPushButton::clicked,
            this,
            &OutputSettings::onChangeFoldersClicked);
    connect(ui->btn_NamingRule, &QPushButton::clicked, this, &OutputSettings::onNamingRuleClicked);
    connect(ui->btn_Restore, &QPushButton::clicked, this, &OutputSettings::onRestoreDefaultClicked);
    connect(GlobalConfig::instance(),
            &GlobalConfig::configChanged,
            this,
            &OutputSettings::onConfigChanged);

    loadSettings();
}

OutputSettings::~OutputSettings()
{
    delete ui;
}

void OutputSettings::loadSettings()
{
    const auto &data = GlobalConfig::instance()->outputData();

    // 加载文件名模式
    ui->lineEdit_FileName->setText(data.fileNameFormat);
    ui->lineEdit_Path->setText(data.fastSavePath);

    // 更新预览
    updatePreview(ui->label_Preview, data.fileNameFormat);
    updatePreview(ui->label_Preview2, data.fastSavePath);
}

void OutputSettings::updatePreview(QLabel *previewLabel, const QString &pattern)
{
    previewLabel->setText(generatePreviewText(pattern));
}

QString OutputSettings::generatePreviewText(const QString &pattern)
{
    QString result = pattern;

    // 获取当前时间
    QDateTime now = QDateTime::currentDateTime();

    // 替换时间相关的占位符
    result.replace("$yyyy", now.toString("yyyy"));
    result.replace("$MM", now.toString("MM"));
    result.replace("$dd", now.toString("dd"));
    result.replace("$HH", now.toString("HH"));
    result.replace("$mm", now.toString("mm"));
    result.replace("$ss", now.toString("ss"));

    // 替换其他可能的占位符
    result.replace("$count", "001");

    return result;
}

void OutputSettings::onManualFileNameChanged(const QString &text)
{
    auto data = GlobalConfig::instance()->outputData();
    data.fileNameFormat = text;
    GlobalConfig::instance()->setOutputData(data);

    // 更新预览
    updatePreview(ui->label_Preview, text);
}

void OutputSettings::onQuickSavePathChanged(const QString &text)
{
    auto data = GlobalConfig::instance()->outputData();
    data.fastSavePath = text;
    GlobalConfig::instance()->setOutputData(data);

    // 更新预览
    updatePreview(ui->label_Preview2, text);
}

void OutputSettings::onOpenFoldersClicked()
{
    const auto &data = GlobalConfig::instance()->outputData();
    QString     path = QFileInfo(data.fastSavePath).absolutePath();

    // 如果目录不存在，创建它
    QDir dir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            QMessageBox::warning(this,
                               tr("警告"),
                               tr("无法创建目录：%1").arg(path));
            return;
        }
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void OutputSettings::onChangeFoldersClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
                                                    tr("选择保存目录"),
                                                    QDir::homePath(),
                                                    QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        auto    data = GlobalConfig::instance()->outputData();
        QString fileName = QFileInfo(data.fastSavePath).fileName();
        data.fastSavePath = QDir(dir).filePath(fileName);
        GlobalConfig::instance()->setOutputData(data);
    }
}

void OutputSettings::onNamingRuleClicked()
{
    QMessageBox::information(this,
                             tr("命名规则说明"),
                             tr("可用的占位符：\n"
                                "$yyyy - 年份(4位数字)\n"
                                "$MM   - 月份(2位数字)\n"
                                "$dd   - 日期(2位数字)\n"
                                "$HH   - 小时(2位数字)\n"
                                "$mm   - 分钟(2位数字)\n"
                                "$ss   - 秒数(2位数字)\n"
                                "$count - 序号(3位数字)"));
}

void OutputSettings::onRestoreDefaultClicked()
{
    // 创建默认设置
    OutputSettingsData defaultData;
    defaultData.fileNameFormat = "QSnapCraft_$yyyy-$MM-$dd_$HH-$mm-$ss.png";
    defaultData.fastSavePath = "FastPictures/QSnapCraft_$yyyy-$MM-$dd_$HH-$mm-$ss.png";

    // 应用默认设置
    GlobalConfig::instance()->setOutputData(defaultData);
}

void OutputSettings::onConfigChanged()
{
    loadSettings();
}
