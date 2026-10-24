#include "outputsettings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QUrl>
#include <QVBoxLayout>

OutputSettings::OutputSettings(QWidget *parent)
    : QWidget(parent)
{
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 22, 24, 22);
    pageLayout->setSpacing(16);

    auto *pathGroup = new QGroupBox(tr("保存位置"), this);
    auto *pathLayout = new QGridLayout(pathGroup);
    pathLayout->setColumnStretch(1, 1);

    const auto addPathRow = [this, pathGroup, pathLayout](int            row,
                                                          const QString &label,
                                                          const QString &objectName,
                                                          QLineEdit    *&lineEdit,
                                                          const QString &buttonText,
                                                          auto           slot) {
        auto *button = new QPushButton(buttonText, pathGroup);
        lineEdit = new QLineEdit(pathGroup);
        lineEdit->setObjectName(objectName);
        lineEdit->setPlaceholderText(tr("留空时使用系统下载文件夹"));
        pathLayout->addWidget(new QLabel(label, pathGroup), row, 0);
        pathLayout->addWidget(lineEdit, row, 1);
        pathLayout->addWidget(button, row, 2);
        connect(button, &QPushButton::clicked, this, slot);
        connect(lineEdit, &QLineEdit::textChanged, this, &OutputSettings::sigChanged);
    };

    addPathRow(0,
               tr("手动保存："),
               QStringLiteral("lineEdit_ManualSavePath"),
               m_manualPath,
               tr("浏览…"),
               &OutputSettings::onBrowseManualPath);
    addPathRow(1,
               tr("快捷保存："),
               QStringLiteral("lineEdit_QuickSavePath"),
               m_quickPath,
               tr("浏览…"),
               &OutputSettings::onBrowseQuickPath);
    addPathRow(2,
               tr("自动保存："),
               QStringLiteral("lineEdit_AutoSavePath"),
               m_autoPath,
               tr("浏览…"),
               &OutputSettings::onBrowseAutoPath);
    pageLayout->addWidget(pathGroup);

    auto *formatGroup = new QGroupBox(tr("文件"), this);
    auto *formatLayout = new QFormLayout(formatGroup);
    formatLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_filenamePattern = new QLineEdit(formatGroup);
    m_filenamePattern->setObjectName(QStringLiteral("lineEdit_FilenamePattern"));
    formatLayout->addRow(tr("文件名规则："), m_filenamePattern);

    m_preview = new QLabel(formatGroup);
    m_preview->setObjectName(QStringLiteral("label_FilenamePreview"));
    m_preview->setTextInteractionFlags(Qt::TextSelectableByMouse);
    formatLayout->addRow(tr("预览："), m_preview);

    auto *formatRow = new QWidget(formatGroup);
    auto *formatRowLayout = new QHBoxLayout(formatRow);
    formatRowLayout->setContentsMargins(0, 0, 0, 0);
    m_imageFormat = new QComboBox(formatRow);
    m_imageFormat->setObjectName(QStringLiteral("combo_ImageFormat"));
    m_imageFormat->addItems({QStringLiteral("PNG"), QStringLiteral("JPG"), QStringLiteral("BMP")});
    formatRowLayout->addWidget(m_imageFormat);
    formatRowLayout->addSpacing(12);
    formatRowLayout->addWidget(new QLabel(tr("JPEG 质量："), formatRow));
    m_jpegQuality = new QSpinBox(formatRow);
    m_jpegQuality->setObjectName(QStringLiteral("spin_JpegQuality"));
    m_jpegQuality->setRange(1, 100);
    m_jpegQuality->setSuffix(QStringLiteral("%"));
    formatRowLayout->addWidget(m_jpegQuality);
    formatRowLayout->addStretch();
    formatLayout->addRow(tr("图片格式："), formatRow);

    auto *namingButton = new QPushButton(tr("查看命名规则"), formatGroup);
    namingButton->setObjectName(QStringLiteral("btn_NamingRule"));
    auto *namingRow = new QHBoxLayout();
    namingRow->addWidget(namingButton);
    auto *openFolderButton = new QPushButton(tr("打开快捷保存文件夹"), formatGroup);
    openFolderButton->setObjectName(QStringLiteral("btn_OpenSaveFolder"));
    namingRow->addWidget(openFolderButton);
    namingRow->addStretch();
    formatLayout->addRow(namingRow);
    pageLayout->addWidget(formatGroup);

    auto *behaviorGroup = new QGroupBox(tr("输出行为"), this);
    auto *behaviorLayout = new QVBoxLayout(behaviorGroup);
    m_autoSave = new QCheckBox(tr("任何输出成功后自动保存副本"), behaviorGroup);
    m_autoSave->setObjectName(QStringLiteral("check_AutoSave"));
    auto *autoSaveHint = new QLabel(tr("复制、贴图、手动保存或快捷保存成功后，都会再写入一份到自动保存目录；"
                                       "建议为自动保存选择独立目录。"),
                                    behaviorGroup);
    autoSaveHint->setObjectName(QStringLiteral("label_AutoSaveHint"));
    autoSaveHint->setWordWrap(true);
    m_soundEnabled = new QCheckBox(tr("输出成功后播放提示音"), behaviorGroup);
    m_soundEnabled->setObjectName(QStringLiteral("check_OutputSound"));
    behaviorLayout->addWidget(m_autoSave);
    behaviorLayout->addWidget(autoSaveHint);
    behaviorLayout->addWidget(m_soundEnabled);
    pageLayout->addWidget(behaviorGroup);
    pageLayout->addStretch();

    connect(m_filenamePattern, &QLineEdit::textChanged, this, [this]() {
        updatePreview();
        emit sigChanged();
    });
    connect(m_imageFormat, &QComboBox::currentIndexChanged, this, [this]() {
        updateQualityState();
        updatePreview();
        emit sigChanged();
    });
    connect(m_jpegQuality, &QSpinBox::valueChanged, this, &OutputSettings::sigChanged);
    connect(m_autoSave, &QCheckBox::toggled, this, &OutputSettings::sigChanged);
    connect(m_soundEnabled, &QCheckBox::toggled, this, &OutputSettings::sigChanged);
    connect(namingButton, &QPushButton::clicked, this, &OutputSettings::onNamingRuleClicked);
    connect(openFolderButton, &QPushButton::clicked, this, &OutputSettings::onOpenFoldersClicked);
}

OutputSettings::~OutputSettings() = default;

void OutputSettings::setPreferences(const ApplicationPreferences &preferences)
{
    const QSignalBlocker manualBlocker(m_manualPath);
    const QSignalBlocker quickBlocker(m_quickPath);
    const QSignalBlocker autoBlocker(m_autoPath);
    const QSignalBlocker patternBlocker(m_filenamePattern);
    const QSignalBlocker formatBlocker(m_imageFormat);
    const QSignalBlocker qualityBlocker(m_jpegQuality);
    const QSignalBlocker autoSaveBlocker(m_autoSave);
    const QSignalBlocker soundBlocker(m_soundEnabled);

    m_manualPath->setText(resolveCaptureSaveDirectory(preferences.capture.manualSavePath));
    m_quickPath->setText(resolveCaptureSaveDirectory(preferences.capture.quickSavePath));
    m_autoPath->setText(resolveCaptureSaveDirectory(preferences.capture.autoSavePath));
    m_filenamePattern->setText(preferences.capture.filenamePattern);
    const int formatIndex = m_imageFormat->findText(preferences.capture.imageFormat, Qt::MatchFixedString);
    m_imageFormat->setCurrentIndex(formatIndex >= 0 ? formatIndex : 0);
    m_jpegQuality->setValue(preferences.capture.jpegQuality);
    m_autoSave->setChecked(preferences.capture.autoSave);
    m_soundEnabled->setChecked(preferences.capture.soundEnabled);
    updateQualityState();
    updatePreview();
}

void OutputSettings::writePreferences(ApplicationPreferences &preferences) const
{
    preferences.capture.manualSavePath = m_manualPath->text().trimmed();
    preferences.capture.quickSavePath = m_quickPath->text().trimmed();
    preferences.capture.autoSavePath = m_autoPath->text().trimmed();
    preferences.capture.filenamePattern = m_filenamePattern->text().trimmed();
    preferences.capture.imageFormat = m_imageFormat->currentText();
    preferences.capture.jpegQuality = m_jpegQuality->value();
    preferences.capture.autoSave = m_autoSave->isChecked();
    preferences.capture.soundEnabled = m_soundEnabled->isChecked();
}

void OutputSettings::restoreDefaults()
{
    ApplicationPreferences preferences;
    setPreferences(preferences);
}

QString OutputSettings::chooseDirectory(const QString &currentPath)
{
    const QString initial = resolveCaptureSaveDirectory(currentPath);
    return QFileDialog::getExistingDirectory(this,
                                             tr("选择保存文件夹"),
                                             initial,
                                             QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
}

void OutputSettings::onBrowseManualPath()
{
    const QString path = chooseDirectory(m_manualPath->text());
    if (!path.isEmpty()) {
        m_manualPath->setText(path);
    }
}

void OutputSettings::onBrowseQuickPath()
{
    const QString path = chooseDirectory(m_quickPath->text());
    if (!path.isEmpty()) {
        m_quickPath->setText(path);
    }
}

void OutputSettings::onBrowseAutoPath()
{
    const QString path = chooseDirectory(m_autoPath->text());
    if (!path.isEmpty()) {
        m_autoPath->setText(path);
    }
}

void OutputSettings::onOpenFoldersClicked()
{
    const QString path = resolveCaptureSaveDirectory(m_quickPath->text());
    QDir().mkpath(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void OutputSettings::onNamingRuleClicked()
{
    QMessageBox::information(this,
                             tr("命名规则"),
                             tr("可用占位符：\n"
                                "$yyyy - 四位年份\n"
                                "$MM - 两位月份\n"
                                "$dd - 两位日期\n"
                                "$HH - 两位小时\n"
                                "$mm - 两位分钟\n"
                                "$ss - 两位秒数"));
}

void OutputSettings::updatePreview()
{
    m_preview->setText(generatePreviewText(m_filenamePattern->text()));
}

void OutputSettings::updateQualityState()
{
    const QString format = m_imageFormat->currentText();
    m_jpegQuality->setEnabled(format == QStringLiteral("JPG") || format == QStringLiteral("JPEG"));
}

QString OutputSettings::generatePreviewText(const QString &pattern) const
{
    QString result = pattern.trimmed();
    if (result.isEmpty()) {
        result = QStringLiteral("QSnapCraft_%1")
                     .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")));
    }
    const QDateTime now = QDateTime::currentDateTime();
    result.replace(QStringLiteral("$yyyy"), now.toString(QStringLiteral("yyyy")));
    result.replace(QStringLiteral("$MM"), now.toString(QStringLiteral("MM")));
    result.replace(QStringLiteral("$dd"), now.toString(QStringLiteral("dd")));
    result.replace(QStringLiteral("$HH"), now.toString(QStringLiteral("HH")));
    result.replace(QStringLiteral("$mm"), now.toString(QStringLiteral("mm")));
    result.replace(QStringLiteral("$ss"), now.toString(QStringLiteral("ss")));
    result.replace(QStringLiteral("$"), QString());
    if (QFileInfo(result).suffix().isEmpty()) {
        result += QStringLiteral(".") + m_imageFormat->currentText().toLower();
    }
    return result;
}
