#pragma once

#include "config/settingsdata.h"

#include <QString>
#include <QWidget>

class QCheckBox;
class QLabel;

class GeneralSettings : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralSettings(QWidget *parent = nullptr);
    ~GeneralSettings() override;

    void setPreferences(const ApplicationPreferences &preferences);
    void writePreferences(ApplicationPreferences &preferences) const;
    void restoreDefaults();
    void setConfigPath(const QString &path);

signals:
    void sigChanged();

private slots:
    void onOpenFolderClicked();
    void onOpenFileClicked();

private:
    QCheckBox *m_autoStart{nullptr};
    QLabel    *m_configPath{nullptr};
    QString    m_path;
};
