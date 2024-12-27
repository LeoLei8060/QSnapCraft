#include "screenshot_window.h"
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>

ScreenshotWindow::ScreenshotWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::WindowTransparentForInput | Qt::Tool);

    connect(&m_mouseHook, &MouseHook::mouseMove, this, &ScreenshotWindow::onMouseMove);
    connect(&m_mouseHook, &MouseHook::buttonLDown, this, &ScreenshotWindow::onLButtonDown);
    connect(&m_mouseHook, &MouseHook::buttonLUp, this, &ScreenshotWindow::onLButtonUp);
    connect(&m_mouseHook, &MouseHook::buttonRDown, this, &ScreenshotWindow::onRButtonDown);
    connect(&m_mouseHook, &MouseHook::buttonRUp, this, &ScreenshotWindow::onRButtonUp);
}

void ScreenshotWindow::start()
{
    int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    setGeometry(left, top, screenWidth, screenHeight);

    captureFullScreens();

    show();
    m_mouseHook.install();
}

void ScreenshotWindow::quit()
{
    m_mouseHook.uninstall();
    hide();
}

void ScreenshotWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawImage(0, 0, m_screenShot);

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
