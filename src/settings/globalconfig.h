#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include <QObject>
#include "generalsettings.h"
#include "controlsettings.h"

class GlobalConfig : public QObject
{
    Q_OBJECT

public:
    static GlobalConfig* instance();

    // 获取各个设置对象的接口
    GeneralSettings* generalSettings() { return &m_generalSettings; }
    ControlSettings* controlSettings() { return &m_controlSettings; }

    // 加载和保存配置
    void loadConfig();
    void saveConfig();

private:
    GlobalConfig(QObject* parent = nullptr);
    ~GlobalConfig();
    
    // 禁用拷贝和赋值
    GlobalConfig(const GlobalConfig&) = delete;
    GlobalConfig& operator=(const GlobalConfig&) = delete;

    static GlobalConfig* m_instance;
    
    // 各个设置对象
    GeneralSettings m_generalSettings;
    ControlSettings m_controlSettings;
};

#endif // GLOBALCONFIG_H
