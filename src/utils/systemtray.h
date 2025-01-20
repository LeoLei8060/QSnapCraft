#pragma once

#include <memory>
#include <QMenu>
#include <QSystemTrayIcon>

class SystemTray : public QObject
{
    Q_OBJECT

public:
    explicit SystemTray(QObject *parent = nullptr);
    ~SystemTray() override = default;

    void show();

signals:
    void sigSettingActTriggered();

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void startScreenshot();

private:
    void createActions();
    void createTrayIcon();

    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
    std::unique_ptr<QMenu>           m_trayMenu;
    QAction                         *m_screenshotAction;
    QAction                         *m_quitAction;
    QAction                         *m_settingAction;
};
