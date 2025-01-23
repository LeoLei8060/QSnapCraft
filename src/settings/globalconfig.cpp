#include "globalconfig.h"
#include <QSettings>

GlobalConfig* GlobalConfig::m_instance = nullptr;

GlobalConfig::GlobalConfig(QObject* parent)
    : QObject(parent)
{
    loadConfig();
}

GlobalConfig::~GlobalConfig()
{
    saveConfig();
}

GlobalConfig* GlobalConfig::instance()
{
    if (!m_instance) {
        m_instance = new GlobalConfig();
    }
    return m_instance;
}

void GlobalConfig::loadConfig()
{
    QSettings settings;
    
    // 加载 GeneralSettings
    settings.beginGroup("General");
    // TODO: 根据 GeneralSettings 类的具体属性加载配置
    settings.endGroup();
    
    // 加载 ControlSettings
    settings.beginGroup("Control");
    // TODO: 根据 ControlSettings 类的具体属性加载配置
    settings.endGroup();
}

void GlobalConfig::saveConfig()
{
    QSettings settings;
    
    // 保存 GeneralSettings
    settings.beginGroup("General");
    // TODO: 根据 GeneralSettings 类的具体属性保存配置
    settings.endGroup();
    
    // 保存 ControlSettings
    settings.beginGroup("Control");
    // TODO: 根据 ControlSettings 类的具体属性保存配置
    settings.endGroup();
    
    settings.sync();
}
