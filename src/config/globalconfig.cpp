#include "globalconfig.h"

#include "preferencessettings.h"

#include <QSettings>

GlobalConfig *GlobalConfig::m_instance = nullptr;

GlobalConfig::GlobalConfig(QObject *parent)
    : QObject(parent)
    , m_configPath(QStringLiteral("config.ini"))
{
    loadConfig();
}

GlobalConfig::~GlobalConfig()
{
    saveConfig();
    if (m_instance == this) {
        m_instance = nullptr;
    }
}

GlobalConfig *GlobalConfig::instance()
{
    if (!m_instance) {
        m_instance = new GlobalConfig();
    }
    return m_instance;
}

void GlobalConfig::setGeneralData(const GeneralSettingsData &data)
{
    ApplicationPreferences preferences = m_preferences;
    preferences.general = data;
    setPreferences(preferences);
}

void GlobalConfig::setControlData(const ControlSettingsData &data)
{
    ApplicationPreferences preferences = m_preferences;
    preferences.control = data;
    setPreferences(preferences);
}

void GlobalConfig::setCaptureBarData(const CaptureBarSettingsData &data)
{
    ApplicationPreferences preferences = m_preferences;
    preferences.captureBar = data;
    setPreferences(preferences);
}

bool GlobalConfig::setPreferences(const ApplicationPreferences &preferences, QString *error)
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    saveApplicationPreferences(settings, preferences);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        if (error != nullptr) {
            *error = QStringLiteral("无法写入配置文件：%1").arg(m_configPath);
        }
        return false;
    }
    m_preferences = preferences;
    emit configChanged();
    return true;
}

QString GlobalConfig::configPath() const
{
    return m_configPath;
}

void GlobalConfig::setConfigPath(const QString &path)
{
    if (path.isEmpty() || m_configPath == path) {
        return;
    }
    m_configPath = path;
    loadConfig();
}

void GlobalConfig::loadConfig()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    m_preferences = loadApplicationPreferences(settings);
}

void GlobalConfig::saveConfig()
{
    QSettings settings(m_configPath, QSettings::IniFormat);
    saveApplicationPreferences(settings, m_preferences);
    settings.sync();
}
