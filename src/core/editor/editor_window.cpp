#include "editor_window.h"

EditorWindow::EditorWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
}

void EditorWindow::setImage(const QPixmap &pixmap)
{
    currentImage = pixmap;
}
