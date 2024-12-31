#include "editor_window.h"
#include <windows.h>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>

EditorWindow::EditorWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Widget | Qt::WindowStaysOnTopHint);

    int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    setGeometry(left, top, screenWidth, screenHeight);
}

void EditorWindow::setImage(const QImage &image)
{
    m_currentImage = image;
}

void EditorWindow::setData(const QImage &image, const QRect &captureRect)
{
    m_currentImage = image;
    m_captureRect = captureRect;
    qDebug() << m_captureRect;
    update();
}

void EditorWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    // 绘制背景截图
    if (m_captureRect.isValid()) {
        painter.drawImage(0, 0, m_currentImage);

        // 添加半透明遮罩
        QPainterPath windowPath;
        windowPath.addRect(rect());

        QPainterPath highlightPath;
        highlightPath.addRect(m_captureRect);

        QPainterPath overlayPath = windowPath.subtracted(highlightPath);

        painter.fillPath(overlayPath, QColor(0, 0, 0, 128));

        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(m_captureRect);
    }

    // 绘制放大镜
    m_magnifier.paint(painter, m_currentImage, QCursor::pos());
}
