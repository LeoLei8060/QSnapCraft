#include "mainwindow.h"
#include "screencapture.h"
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , screenCapture(new ScreenCapture(this))
{
    setupUI();
    createTrayIcon();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    captureBtn = new QPushButton(tr("Capture Widget"), this);
    layout->addWidget(captureBtn);
    
    connect(captureBtn, &QPushButton::clicked, this, &MainWindow::startCapture);
    
    setWindowTitle(tr("QSnapCraft"));
    resize(200, 100);
}

void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIcon = new QSystemTrayIcon(this);
    
    QAction *captureAction = new QAction(tr("Capture Widget"), this);
    QAction *quitAction = new QAction(tr("Quit"), this);
    
    connect(captureAction, &QAction::triggered, this, &MainWindow::startCapture);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    
    trayIconMenu->addAction(captureAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
    
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/icons/app.png")); // 需要添加资源文件
    trayIcon->show();
    
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);
}

void MainWindow::startCapture()
{
    hide();
    QTimer::singleShot(200, this, [this]() {
        screenCapture->startCapture();
    });
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        if (isVisible())
            hide();
        else
            show();
    }
}
