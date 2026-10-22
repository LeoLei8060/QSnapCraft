#pragma once

#include "app/applicationaction.h"

#include <memory>
#include <QMenu>
#include <QSystemTrayIcon>

class QAction;

class SystemTray : public QObject
{
    Q_OBJECT

public:
    explicit SystemTray(QObject *parent = nullptr);
    ~SystemTray() override = default;

    void show();
    void closeContextMenu();
    void setCaptureBarVisible(bool visible);
    QMenu *contextMenu() const;

signals:
    void sigActionTriggered(ApplicationAction action);

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onActionTriggered();

private:
    QAction *addAction(ApplicationAction action, bool checkable = false);
    void createTrayIcon();

    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
    std::unique_ptr<QMenu>           m_trayMenu;
    QAction                         *m_toggleCaptureBarAction{nullptr};
};
