#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

class ScreenCapture;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startCapture();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void createActions();
    void createTrayIcon();
    void setupUI();

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QPushButton *captureBtn;
    ScreenCapture *screenCapture;
};

#endif // MAINWINDOW_H
