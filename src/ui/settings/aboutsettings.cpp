#include "aboutsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QVBoxLayout>

AboutSettings::AboutSettings(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 30, 32, 30);
    layout->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("QSnapCraft"), this);
    title->setObjectName(QStringLiteral("label_AboutTitle"));
    title->setStyleSheet(QStringLiteral("font-size: 26px; font-weight: 600;"));
    layout->addWidget(title);

    const QString application_version = QCoreApplication::applicationVersion().isEmpty()
                                            ? QStringLiteral("1.0.0")
                                            : QCoreApplication::applicationVersion();
    auto         *version = new QLabel(tr("版本 %1").arg(application_version), this);
    version->setObjectName(QStringLiteral("label_AboutVersion"));
    layout->addWidget(version);

    auto *description = new QLabel(tr("轻量、快速的 Windows 截图与屏幕工具。"), this);
    description->setWordWrap(true);
    layout->addWidget(description);
    layout->addSpacing(16);

    auto *runtime = new QLabel(tr("Qt 版本：%1").arg(QString::fromLatin1(qVersion())), this);
    runtime->setObjectName(QStringLiteral("label_AboutQtVersion"));
    layout->addWidget(runtime);

    m_configPath = new QLabel(this);
    m_configPath->setObjectName(QStringLiteral("label_AboutConfigPath"));
    m_configPath->setWordWrap(true);
    m_configPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_configPath);
    layout->addStretch();
}

AboutSettings::~AboutSettings() = default;

void AboutSettings::setPreferences(const ApplicationPreferences &preferences)
{
    Q_UNUSED(preferences)
}

void AboutSettings::writePreferences(ApplicationPreferences &preferences) const
{
    Q_UNUSED(preferences)
}

void AboutSettings::restoreDefaults() {}

void AboutSettings::setConfigPath(const QString &path)
{
    m_configPath->setText(tr("配置文件：%1").arg(QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath())));
}
