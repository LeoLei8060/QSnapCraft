#include "pin_window.h"

PinWindow::PinWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
}

void PinWindow::setImage(const QPixmap &pixmap)
{
    currentImage = pixmap;
}
