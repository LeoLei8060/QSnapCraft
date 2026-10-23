#pragma once

#include <memory>
#include <QObject>

class SettingsWindow;

class WindowManager : public QObject
{
    Q_OBJECT

public:
    explicit WindowManager(QObject *parent = nullptr);
    ~WindowManager() override;

public slots:
    void onSettingActTriggered();
    void onAboutActTriggered();

private:
    void                            showSettingsWindow();
    std::unique_ptr<SettingsWindow> m_settingsWindow;
};
