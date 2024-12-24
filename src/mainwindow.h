#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include "uiautomation.h"
#include "screencapture.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void refreshWindows();
    void onWindowSelected(QListWidgetItem* item);
    void startScreenshot();
    void onCaptureCompleted(const QPixmap &pixmap);

private:
    void setupUI();
    void updateControlsInfo(const WindowInfo& info);

    QListWidget* windowList;
    QTreeWidget* controlsTree;
    QPushButton* refreshButton;
    QPushButton* screenshotButton;
    UIAutomation* automation;
    ScreenCapture* screencapture;
};

#endif // MAINWINDOW_H
