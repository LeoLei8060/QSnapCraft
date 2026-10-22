#include "generalsettings.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QUrl>
#include <QVBoxLayout>

GeneralSettings::GeneralSettings(QWidget *parent)
    : QWidget(parent)
{
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 22, 24, 22);
    pageLayout->setSpacing(16);

    auto *startupGroup = new QGroupBox(tr("启动"), this);
    auto *startupLayout = new QVBoxLayout(startupGroup);
    m_autoStart = new QCheckBox(tr("开机启动 QSnapCraft"), startupGroup);
    m_autoStart->setObjectName(QStringLiteral("check_AutoStart"));
    startupLayout->addWidget(m_autoStart);
    auto *startupHint = new QLabel(tr("登录 Windows 后自动在系统托盘运行。"), startupGroup);
    startupHint->setObjectName(QStringLiteral("label_StartupHint"));
    startupHint->setWordWrap(true);
    startupLayout->addWidget(startupHint);
    pageLayout->addWidget(startupGroup);

    auto *configGroup = new QGroupBox(tr("配置文件"), this);
    auto *configLayout = new QVBoxLayout(configGroup);
    m_configPath = new QLabel(configGroup);
    m_configPath->setObjectName(QStringLiteral("label_ConfigPath"));
    m_configPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_configPath->setWordWrap(true);
    configLayout->addWidget(m_configPath);

    auto *buttonRow = new QHBoxLayout();
    auto *openFileButton = new QPushButton(tr("打开配置文件"), configGroup);
    openFileButton->setObjectName(QStringLiteral("btn_OpenConfigFile"));
    auto *openFolderButton = new QPushButton(tr("打开所在文件夹"), configGroup);
    openFolderButton->setObjectName(QStringLiteral("btn_OpenConfigFolder"));
    buttonRow->addWidget(openFileButton);
    buttonRow->addWidget(openFolderButton);
    buttonRow->addStretch();
    configLayout->addLayout(buttonRow);
    pageLayout->addWidget(configGroup);
    pageLayout->addStretch();

    connect(m_autoStart, &QCheckBox::toggled, this, &GeneralSettings::sigChanged);
    connect(openFileButton, &QPushButton::clicked, this, &GeneralSettings::onOpenFileClicked);
    connect(openFolderButton, &QPushButton::clicked, this, &GeneralSettings::onOpenFolderClicked);
}

GeneralSettings::~GeneralSettings() = default;

void GeneralSettings::setPreferences(const ApplicationPreferences &preferences)
{
    const QSignalBlocker blocker(m_autoStart);
    m_autoStart->setChecked(preferences.general.autoStart);
}

void GeneralSettings::writePreferences(ApplicationPreferences &preferences) const
{
    preferences.general.autoStart = m_autoStart->isChecked();
}

void GeneralSettings::restoreDefaults()
{
    const GeneralSettingsData defaults;
    const QSignalBlocker      blocker(m_autoStart);
    m_autoStart->setChecked(defaults.autoStart);
}

void GeneralSettings::setConfigPath(const QString &path)
{
    m_path = QFileInfo(path).absoluteFilePath();
    m_configPath->setText(QDir::toNativeSeparators(m_path));
}

void GeneralSettings::onOpenFolderClicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_path).absolutePath()));
}

void GeneralSettings::onOpenFileClicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_path));
}
