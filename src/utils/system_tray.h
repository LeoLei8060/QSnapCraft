#pragma once

#include <QSystemTrayIcon>
#include <QMenu>
#include <memory>

class SystemTray : public QObject {
    Q_OBJECT

public:
    explicit SystemTray(QObject* parent = nullptr);
    ~SystemTray() override = default;

    void show();

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void startScreenshot();

private:
    void createActions();
    void createTrayIcon();

    std::unique_ptr<QSystemTrayIcon> trayIcon;
    std::unique_ptr<QMenu> trayMenu;
    QAction* screenshotAction;
    QAction* quitAction;
};
