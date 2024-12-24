#include "mainwindow.h"
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    automation = new UIAutomation();
    //    refreshWindows();
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
    windowList = new QListWidget(this);
    refreshButton = new QPushButton("Refresh", this);
    screenshotButton = new QPushButton("Screenshot", this);
    leftLayout->addWidget(windowList);
    leftLayout->addWidget(refreshButton);
    leftLayout->addWidget(screenshotButton);

    // Right side - controls tree
    controlsTree = new QTreeWidget(this);
    controlsTree->setHeaderLabels({"Control Type", "Name", "Position", "Size", "Value", "Enabled"});
    controlsTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addWidget(controlsTree, 2);

    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshWindows);
    connect(windowList, &QListWidget::itemClicked, this, &MainWindow::onWindowSelected);
    connect(screenshotButton, &QPushButton::clicked, this, &MainWindow::startScreenshot);

    resize(1200, 800);
}

void MainWindow::refreshWindows()
{
    windowList->clear();
    controlsTree->clear();

    auto windows = automation->getWindows();
    for (const auto &window : windows) {
        QListWidgetItem *item = new QListWidgetItem(window.title);
        item->setData(Qt::UserRole, reinterpret_cast<qulonglong>(window.handle));
        windowList->addItem(item);
    }
}

void MainWindow::onWindowSelected(QListWidgetItem *item)
{
    if (!item)
        return;

    HWND       handle = reinterpret_cast<HWND>(item->data(Qt::UserRole).toULongLong());
    WindowInfo info = automation->getWindowControls(handle);
    updateControlsInfo(info);
}

void MainWindow::updateControlsInfo(const WindowInfo &info)
{
    controlsTree->clear();

    for (const auto &control : info.controls) {
        QTreeWidgetItem *item = new QTreeWidgetItem(controlsTree);
        item->setText(0, control.type);
        item->setText(1, control.name);
        item->setText(2, QString("(%1, %2)").arg(control.bounds.x()).arg(control.bounds.y()));
        item->setText(3, QString("%1x%2").arg(control.bounds.width()).arg(control.bounds.height()));
        item->setText(4, control.value);
        item->setText(5, control.isEnabled ? "Yes" : "No");
    }
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
