#include "screenshot_window.h"
#include <windows.h>
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>

ScreenshotWindow::ScreenshotWindow(QWidget *parent)
    : QWidget(parent)
    , m_magnifier(new Magnifier)
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

ScreenshotWindow::~ScreenshotWindow()
{
    delete m_magnifier;
}

void ScreenshotWindow::start()
{
    int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    setGeometry(left, top, screenWidth, screenHeight);

    // 捕获全屏图像
    captureFullScreens();

    show();
    m_mouseHook.install();
}

void ScreenshotWindow::quit()
{
    // 退出截图工具
    m_mouseHook.uninstall();
    hide();
}

void ScreenshotWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    // 绘制背景截图
    painter.drawImage(0, 0, m_screenShot);

    // 添加半透明遮罩
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

    // 绘制放大镜
    m_magnifier->paint(painter, m_screenShot, QCursor::pos());
}

void ScreenshotWindow::captureFullScreens()
{
    int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) {
        qDebug() << "Failed to get screen DC";
        return;
    }

    HDC hdcMemory = CreateCompatibleDC(hdcScreen);
    if (!hdcMemory) {
        ReleaseDC(NULL, hdcScreen);
        qDebug() << "Failed to create memory DC";
        return;
    }

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
    if (!hBitmap) {
        DeleteDC(hdcMemory);
        ReleaseDC(NULL, hdcScreen);
        qDebug() << "Failed to create bitmap";
        return;
    }

    HBITMAP hOldBitmap = (HBITMAP) SelectObject(hdcMemory, hBitmap);

    if (!BitBlt(hdcMemory, 0, 0, screenWidth, screenHeight, hdcScreen, left, top, SRCCOPY)) {
        SelectObject(hdcMemory, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemory);
        ReleaseDC(NULL, hdcScreen);
        qDebug() << "BitBlt failed";
        return;
    }

    BITMAP bmpScreen;
    GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);

    BITMAPINFOHEADER bi;
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = -bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    int                     dataSize = bmpScreen.bmWidth * 4 * bmpScreen.bmHeight;
    std::unique_ptr<BYTE[]> lpBits(new BYTE[dataSize]);

    if (!GetDIBits(hdcMemory,
                   hBitmap,
                   0,
                   bmpScreen.bmHeight,
                   lpBits.get(),
                   (BITMAPINFO *) &bi,
                   DIB_RGB_COLORS)) {
        SelectObject(hdcMemory, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemory);
        ReleaseDC(NULL, hdcScreen);
        qDebug() << "GetDIBits failed";
        return;
    }

    m_screenShot = QImage(bmpScreen.bmWidth, bmpScreen.bmHeight, QImage::Format_ARGB32);
    memcpy(m_screenShot.bits(), lpBits.get(), dataSize);

    SelectObject(hdcMemory, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMemory);
    ReleaseDC(NULL, hdcScreen);
}

void ScreenshotWindow::onMouseMove(const POINT &pt)
{
    m_highlightRect = m_inspector.quickInspect(pt);
    update();
}

void ScreenshotWindow::onLButtonDown(const POINT &pt) {}

void ScreenshotWindow::onLButtonUp(const POINT &pt) {}

void ScreenshotWindow::onRButtonDown(const POINT &pt) {}

void ScreenshotWindow::onRButtonUp(const POINT &pt)
{
    quit();
}
