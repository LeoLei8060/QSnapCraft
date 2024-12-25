#include "mainwindow.h"
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    automation = new UIAutomation();
}

MainWindow::~MainWindow()
{
    delete automation;
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Left side - window list
    QVBoxLayout *leftLayout = new QVBoxLayout();
    screenshotButton = new QPushButton("Screenshot", this);
    leftLayout->addWidget(screenshotButton);

    mainLayout->addLayout(leftLayout, 1);

    connect(screenshotButton, &QPushButton::clicked, this, &MainWindow::startScreenshot);
}

void MainWindow::startScreenshot()
{
    screencapture = new ScreenCapture(this);
    connect(screencapture, &ScreenCapture::captureCompleted, this, &MainWindow::onCaptureCompleted);
    connect(screencapture,
            &ScreenCapture::captureCanceled,
            screencapture,
            &ScreenCapture::deleteLater);
    screencapture->start();
}

void MainWindow::onCaptureCompleted(const QPixmap &pixmap)
{
    // 保存截图
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Screenshot"),
                                                    "",
                                                    tr("Images (*.png *.jpg)"));

    if (!fileName.isEmpty()) {
        pixmap.save(fileName);
    }

    screencapture->deleteLater();
}
