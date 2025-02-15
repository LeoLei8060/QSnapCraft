#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include "settingsdata.h"
#include <QObject>

class GlobalConfig : public QObject
{
    Q_OBJECT

public:
    ~GlobalConfig();

    static GlobalConfig *instance();

    // 获取配置数据的接口
    GeneralSettingsData    &generalData() { return m_generalData; }
    ControlSettingsData    &controlData() { return m_controlData; }
    OutputSettingsData     &outputData() { return m_outputData; }
    ScreenshotSettingsData &screenshotData() { return m_screenshotData; }

    // 修改配置数据的接口
    void setGeneralData(const GeneralSettingsData &data);
    void setControlData(const ControlSettingsData &data);
    void setOutputData(const OutputSettingsData &data);
    void setScreenshotData(const ScreenshotSettingsData &data);

    // 加载和保存配置
    void loadConfig();
    void saveConfig();

signals:
    // 配置改变信号
    void configChanged();

private:
    GlobalConfig(QObject *parent = nullptr);

    GlobalConfig(const GlobalConfig &) = delete;
    GlobalConfig &operator=(const GlobalConfig &) = delete;

    static GlobalConfig *m_instance;

    // 配置数据
    GeneralSettingsData    m_generalData;
    ControlSettingsData    m_controlData;
    OutputSettingsData     m_outputData;
    ScreenshotSettingsData m_screenshotData;
};

#endif // GLOBALCONFIG_H
