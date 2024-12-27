#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::WindowTransparentForInput);

    QScreen *screen = QApplication::primaryScreen();
    setGeometry(screen->geometry());

    m_screenShot = screen->grabWindow(0);

    connect(&m_mouseHook, &MouseHook::mouseMove, this, &MainWindow::onMouseMove);
    connect(&m_mouseHook, &MouseHook::buttonLDown, this, &MainWindow::onLButtonDown);
    connect(&m_mouseHook, &MouseHook::buttonLUp, this, &MainWindow::onLButtonUp);
    connect(&m_mouseHook, &MouseHook::buttonRDown, this, &MainWindow::onRButtonDown);
    connect(&m_mouseHook, &MouseHook::buttonRUp, this, &MainWindow::onRButtonUp);

    m_mouseHook.install();
}

MainWindow::~MainWindow() {}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_screenShot);

    if (m_highlightRect.isValid()) {
        QPainterPath windowPath;
        windowPath.addRect(rect());

        QPainterPath highlightPath;
        highlightPath.addRect(m_highlightRect);

        QPainterPath overlayPath = windowPath.subtracted(highlightPath);

        painter.fillPath(overlayPath, QColor(0, 0, 0, 128));

        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(m_highlightRect);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    POINT pt;
    GetCursorPos(&pt);
    m_inspector.quickInspect(pt);
}

void MainWindow::onMouseMove(const POINT &pt)
{
    m_highlightRect = m_inspector.quickInspect(pt);
    update();
}

void MainWindow::onLButtonDown(const POINT &pt)
{
    qDebug() << __FUNCTION__;
    // TODO: 记录鼠标左键按下的坐标
}

void MainWindow::onLButtonUp(const POINT &pt)
{
    qDebug() << __FUNCTION__;
    // TODO:
}

void MainWindow::onRButtonDown(const POINT &pt)
{
    qDebug() << __FUNCTION__;
}

void MainWindow::onRButtonUp(const POINT &pt)
{
    qDebug() << __FUNCTION__;
    this->close();
}
