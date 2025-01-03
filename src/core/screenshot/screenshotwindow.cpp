#include "screenshotwindow.h"
#include <windows.h>
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>

ScreenshotWindow::ScreenshotWindow(QWidget *parent)
    : QWidget(parent)
    , m_isDragging(false)
    , m_smartInspect(false)
{
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::WindowTransparentForInput | Qt::CoverWindow);

    connect(&m_mouseHook, &MouseHook::mouseMove, this, &ScreenshotWindow::onMouseMove);
    connect(&m_mouseHook, &MouseHook::buttonLDown, this, &ScreenshotWindow::onLButtonDown);
    connect(&m_mouseHook, &MouseHook::buttonLUp, this, &ScreenshotWindow::onLButtonUp);
    connect(&m_mouseHook, &MouseHook::buttonRDown, this, &ScreenshotWindow::onRButtonDown);
    connect(&m_mouseHook, &MouseHook::buttonRUp, this, &ScreenshotWindow::onRButtonUp);
}

ScreenshotWindow::~ScreenshotWindow() {}

void ScreenshotWindow::start(const QPixmap &pixmap,
                             const QImage  &img,
                             const int     &left,
                             const int     &top,
                             const int     &width,
                             const int     &height)
{
    m_screenshotPixmap = pixmap;
    m_screenshotImg = img;
    setGeometry(left, top, width, height);

    activateScreenCapture();
    show();
}

void ScreenshotWindow::quit()
{
    // 退出截图工具
    m_smartInspect = false;
    m_mouseHook.uninstall();
    hide();
    emit sigCancelScreenshot();
}

void ScreenshotWindow::showWindow()
{
    activateScreenCapture();
    show();
}

void ScreenshotWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_screenshotPixmap);

    // 绘制背景截图
    if (m_highlightRect.isValid()) {
        // 添加半透明遮罩
        QPainterPath windowPath;
        windowPath.addRect(rect());

        QPainterPath highlightPath;
        highlightPath.addRect(m_highlightRect);

        QPainterPath overlayPath = windowPath.subtracted(highlightPath);

        painter.fillPath(overlayPath, QColor(0, 0, 0, 128));

        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(m_highlightRect);
    }

    // 绘制放大镜
    m_magnifier.paint(painter, m_screenshotPixmap, m_screenshotImg, QCursor::pos());
}

void ScreenshotWindow::onMouseMove(const POINT &pt)
{
    if (m_isDragging) {
        // 如果正在拖拽，更新选择区域
        QPoint currentPos(pt.x, pt.y);
        m_highlightRect = QRect(m_dragStartPos, currentPos).normalized();
    } else if (m_smartInspect) {
        // 智能检测模式下进行UI组件检测
        m_highlightRect = m_inspector.quickInspect(pt);
    }
    update();
}

void ScreenshotWindow::onLButtonDown(const POINT &pt)
{
    // 记录拖拽起始位置
    m_dragStartPos = QPoint(pt.x, pt.y);
    m_isDragging = true;

    // 关闭智能检测
    m_smartInspect = false;

    update();
}

void ScreenshotWindow::onLButtonUp(const POINT &pt)
{
    if (m_isDragging) {
        m_isDragging = false;
        // 完成拖拽，更新最终区域
        QPoint endPos(pt.x, pt.y);
        if (endPos != m_dragStartPos) {
            m_highlightRect = QRect(m_dragStartPos, endPos).normalized();
        }
        m_shotRect = m_highlightRect;
        qDebug() << __FUNCTION__ << m_shotRect << m_highlightRect;
        // 切换到编辑状态
        activateScreenEdit();
    }
}

void ScreenshotWindow::onRButtonDown(const POINT &pt)
{
    quit();
}

void ScreenshotWindow::onRButtonUp(const POINT &pt)
{
    //    quit();
}

void ScreenshotWindow::activateScreenCapture()
{
    //    setWindowFlags(windowFlags() | Qt::WindowTransparentForInput);
    qDebug() << __FUNCTION__;
    m_shotRect = QRect();
    //    m_highlightRect = QRect();

    // 开启智能检测
    m_smartInspect = true;
    // 安装钩子
    m_mouseHook.install();

    const auto &pt = QCursor::pos();
    POINT       point;
    point.x = pt.x();
    point.y = pt.y();
    onMouseMove(point);
}

void ScreenshotWindow::activateScreenEdit()
{
    qDebug() << __FUNCTION__ << m_shotRect;
    m_smartInspect = false;

    m_mouseHook.uninstall();

    emit sigCompleteScreenshot();
    hide();
}
