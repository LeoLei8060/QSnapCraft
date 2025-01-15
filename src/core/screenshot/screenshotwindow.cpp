#include "screenshotwindow.h"
#include <windows.h>
#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>

// 定义所有需要修改的系统光标ID
const DWORD ScreenshotWindow::CURSOR_IDS[] = {
    32512, // OCR_NORMAL / 标准箭头
    32513, // OCR_IBEAM / 文本编辑
    32514, // OCR_WAIT / 等待
    32515, // OCR_CROSS / 十字
    32516, // OCR_UP / 向上箭头
    32642, // OCR_SIZENWSE / 左上-右下调整
    32643, // OCR_SIZENESW / 右上-左下调整
    32644, // OCR_SIZEWE / 左右调整
    32645, // OCR_SIZENS / 上下调整
    32646, // OCR_SIZEALL / 移动
    32648, // OCR_NO / 禁止
    32649, // OCR_HAND / 手型
    32650, // OCR_APPSTARTING / 后台工作
    32651  // OCR_HELP / 帮助
};

const int ScreenshotWindow::CURSOR_COUNT = sizeof(CURSOR_IDS) / sizeof(CURSOR_IDS[0]);

ScreenshotWindow::ScreenshotWindow(QWidget *parent)
    : QWidget(parent)
    , m_isDragging(false)
    , m_smartInspect(false)
{
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    m_crossCursor = LoadCursor(NULL, IDC_CROSS);

    connect(&m_mouseHook, &MouseHook::mouseMove, this, &ScreenshotWindow::onMouseMove);
    connect(&m_mouseHook, &MouseHook::buttonLDown, this, &ScreenshotWindow::onLButtonDown);
    connect(&m_mouseHook, &MouseHook::buttonLUp, this, &ScreenshotWindow::onLButtonUp);
    connect(&m_mouseHook, &MouseHook::buttonRDown, this, &ScreenshotWindow::onRButtonDown);
    connect(&m_mouseHook, &MouseHook::buttonRUp, this, &ScreenshotWindow::onRButtonUp);
}

ScreenshotWindow::~ScreenshotWindow()
{
    restoreSystemCursor();
}

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

void ScreenshotWindow::mouseMoveEvent(QMouseEvent *event)
{
    QPoint currentPos = QCursor::pos();

    if (m_isDragging) {
        // 如果正在拖拽，更新选择区域
        m_highlightRect = QRect(m_dragStartPos, currentPos).normalized();
    } else if (m_smartInspect) {
        // 临时设置窗口为完全透明并允许鼠标穿透
        HWND hwnd = (HWND) winId();
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

        // 智能检测模式下进行UI组件检测
        m_highlightRect = m_inspector.quickInspect(currentPos);

        // 恢复窗口样式
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
    }
    update();
}

void ScreenshotWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 记录拖拽起始位置
        m_dragStartPos = QCursor::pos();
        m_isDragging = true;

        // 关闭智能检测
        m_smartInspect = false;

        update();
    } else if (event->button() == Qt::RightButton) {
        quit();
    }
}

void ScreenshotWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_isDragging) {
            m_isDragging = false;
            // 完成拖拽，更新最终区域
            QPoint endPos = QCursor::pos();
            if (endPos != m_dragStartPos) {
                m_highlightRect = QRect(m_dragStartPos, endPos).normalized();
            }
            m_shotRect = m_highlightRect;
            qDebug() << __FUNCTION__ << m_shotRect << m_highlightRect;
            // 切换到编辑状态
            activateScreenEdit();
        }
    }
}

bool ScreenshotWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_SETCURSOR) {
        // 强制设置光标为 IDC_CROSS
        SetCursor(LoadCursor(nullptr, IDC_CROSS));
        *result = TRUE; // 告诉系统已处理消息
        return true;
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void ScreenshotWindow::onMouseMove(const POINT &pt)
{
    if (m_isDragging) {
        // 如果正在拖拽，更新选择区域
        QPoint currentPos(pt.x, pt.y);
        m_highlightRect = QRect(m_dragStartPos, currentPos).normalized();
    } else if (m_smartInspect) {
        // 临时设置窗口为完全透明并允许鼠标穿透
        HWND hwnd = (HWND) winId();
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

        // 智能检测模式下进行UI组件检测
        m_highlightRect = m_inspector.quickInspect(pt);

        // 恢复窗口样式
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
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
    m_shotRect = QRect();

    // 开启智能检测
    m_smartInspect = true;
    // 安装钩子
    m_mouseHook.install();

    setSystemCursor();

    const auto &pt = QCursor::pos();
    POINT       point;
    point.x = pt.x();
    point.y = pt.y();
    onMouseMove(point);
}

void ScreenshotWindow::activateScreenEdit()
{
    m_smartInspect = false;

    m_mouseHook.uninstall();

    restoreSystemCursor();

    emit sigCompleteScreenshot();
    hide();
}

void ScreenshotWindow::setSystemCursor()
{
    for (int i = 0; i < CURSOR_COUNT; ++i) {
        HCURSOR copy = CopyCursor(m_crossCursor);
        SetSystemCursor(copy, CURSOR_IDS[i]);
    }
}

void ScreenshotWindow::restoreSystemCursor()
{
    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
}
