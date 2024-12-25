#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "screencapture.h"
#include "uiautomation.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startScreenshot();
    void onCaptureCompleted(const QPixmap &pixmap);

private:
    void setupUI();

    QPushButton   *screenshotButton;
    UIAutomation  *automation;
    ScreenCapture *screencapture;
};

#endif // MAINWINDOW_H
