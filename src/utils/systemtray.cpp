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
    trayIcon->show();
}

void SystemTray::createActions()
{
    screenshotAction = new QAction(tr("Take Screenshot"), this);
    connect(screenshotAction, &QAction::triggered, this, &SystemTray::startScreenshot);

    quitAction = new QAction(tr("Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void SystemTray::createTrayIcon()
{
    trayMenu = std::make_unique<QMenu>();
    trayMenu->addAction(screenshotAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

    trayIcon = std::make_unique<QSystemTrayIcon>();
    trayIcon->setContextMenu(trayMenu.get());
    trayIcon->setIcon(QIcon(":/icons/QSnapCraft.ico"));
    trayIcon->setToolTip("QSnapCraft");

    connect(trayIcon.get(), &QSystemTrayIcon::activated, this, &SystemTray::onTrayIconActivated);
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
