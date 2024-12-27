#include "screenshot_window.h"
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>

ScreenshotWindow::ScreenshotWindow(QWidget *parent)
    : QWidget(parent)
{}

void ScreenshotWindow::start()
{
    qDebug() << __FUNCTION__;
}
