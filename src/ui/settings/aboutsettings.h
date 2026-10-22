#pragma once

#include "config/settingsdata.h"

#include <QString>
#include <QWidget>

class QLabel;

class AboutSettings : public QWidget
{
    Q_OBJECT

public:
    explicit AboutSettings(QWidget *parent = nullptr);
    ~AboutSettings() override;

    void setPreferences(const ApplicationPreferences &preferences);
    void writePreferences(ApplicationPreferences &preferences) const;
    void restoreDefaults();
    void setConfigPath(const QString &path);

private:
    QLabel *m_configPath{nullptr};
};
