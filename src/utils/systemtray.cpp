#include "systemtray.h"
#include <QApplication>

SystemTray::SystemTray(QObject *parent)
    : QObject(parent)
{
    createActions();
    createTrayIcon();
}

void SystemTray::show()
{
    m_trayIcon->show();
}

void SystemTray::createActions()
{
    m_screenshotAction = new QAction(tr("Take Screenshot"), this);
    connect(m_screenshotAction, &QAction::triggered, this, &SystemTray::startScreenshot);

    m_quitAction = new QAction(tr("Quit"), this);
    connect(m_quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_settingAction = new QAction(tr("setting"), this);
    connect(m_settingAction, &QAction::triggered, this, &SystemTray::sigSettingActTriggered);
}

void SystemTray::createTrayIcon()
{
    m_trayMenu = std::make_unique<QMenu>();
    m_trayMenu->addAction(m_settingAction);
    m_trayMenu->addAction(m_screenshotAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quitAction);

    m_trayIcon = std::make_unique<QSystemTrayIcon>();
    m_trayIcon->setContextMenu(m_trayMenu.get());
    m_trayIcon->setIcon(QIcon(":/icons/QSnapCraft.ico"));
    m_trayIcon->setToolTip("QSnapCraft");

    connect(m_trayIcon.get(), &QSystemTrayIcon::activated, this, &SystemTray::onTrayIconActivated);
}

void SystemTray::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        startScreenshot();
    }
}

void SystemTray::startScreenshot()
{
    //    emit screenshotAction->triggered();
}
