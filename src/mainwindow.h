#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mousehook.h"
#include "uiinspector.h"
#include <QMainWindow>
#include <QPixmap>
#include <QScreen>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onMouseMove(const POINT &pt);
    void onLButtonDown(const POINT &pt);
    void onLButtonUp(const POINT &pt);
    void onRButtonDown(const POINT &pt);
    void onRButtonUp(const POINT &pt);

private:
    QPixmap     m_screenShot;
    UIInspector m_inspector;
    MouseHook   m_mouseHook;

    QRect m_highlightRect;
};

#endif
