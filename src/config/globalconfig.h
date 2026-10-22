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
    const GeneralSettingsData    &generalData() const { return m_preferences.general; }
    const ControlSettingsData    &controlData() const { return m_preferences.control; }
    const CaptureBarSettingsData &captureBarData() const { return m_preferences.captureBar; }
    const ApplicationPreferences &getPreferences() const { return m_preferences; }

    // 修改配置数据的接口
    void setGeneralData(const GeneralSettingsData &data);
    void setControlData(const ControlSettingsData &data);
    void setCaptureBarData(const CaptureBarSettingsData &data);
    bool setPreferences(const ApplicationPreferences &preferences, QString *error = nullptr);

    QString configPath() const;
    void    setConfigPath(const QString &path);

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

    QString              m_configPath;
    ApplicationPreferences m_preferences;
};

#endif // GLOBALCONFIG_H
