#include "WindowManager.h"
#include <QDebug>

WindowManager::WindowManager(QObject *parent)
    : QObject(parent)
{
    m_screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    m_screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    m_screenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    m_screenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);

    qDebug() << __FUNCTION__ << m_screenLeft << m_screenTop << m_screenWidth << m_screenHeight;

    // 创建窗口实例
    m_screenshotWindow = std::make_unique<ScreenshotWindow>();
    m_editorWindow = std::make_unique<EditorWindow>();

    // 连接信号
    connect(m_screenshotWindow.get(),
            &ScreenshotWindow::sigCompleteScreenshot,
            this,
            &WindowManager::onCompleteScreenshot);
    connect(m_screenshotWindow.get(),
            &ScreenshotWindow::sigCancelScreenshot,
            this,
            &WindowManager::onCancelScreenshot);
    connect(m_editorWindow.get(),
            &EditorWindow::sigEditorFinished,
            this,
            &WindowManager::onCompleteEditor);
    connect(m_editorWindow.get(),
            &EditorWindow::sigCancelEditor,
            this,
            &WindowManager::onCancelEditor);
}

WindowManager::~WindowManager() {}

void WindowManager::startCapture()
{
    if (m_state != State::Idle) {
        return;
    }

    captureFullScreens();

    m_originalCursor = GetCursor();

    m_state = State::Capturing;
    m_screenshotWindow->start(m_screenshotPixmap,
                              m_screenshotImg,
                              m_screenLeft,
                              m_screenTop,
                              m_screenWidth,
                              m_screenHeight);
}

void WindowManager::finishCapture()
{
    switchToIdle();
}

void WindowManager::startEdit()
{
    onCancelScreenshot();
}

void WindowManager::onCancelScreenshot()
{
    if (m_state != State::Capturing)
        return;
    m_state = State::Idle;
    if (m_screenshotWindow)
        m_screenshotWindow->hide();
    if (m_editorWindow)
        m_editorWindow->hide();

    SetCursor(m_originalCursor);
}

void WindowManager::onCompleteScreenshot()
{
    if (m_state != State::Capturing)
        return;

    // 获取截图数据
    QImage image = m_screenshotWindow->getCaptureImage().toImage();
    QRect  rect = m_screenshotWindow->getCaptureRect();

    // 切换到编辑状态
    switchToEdit();

    // 设置编辑窗口数据
    m_editorWindow->start(image, rect);
    m_editorWindow->show();
}

void WindowManager::onCancelEditor()
{
    if (m_state != State::Editing)
        return;
    m_state = State::Capturing;
    if (m_editorWindow)
        m_editorWindow->hideWindow();
    if (m_screenshotWindow)
        m_screenshotWindow->showWindow();
}

void WindowManager::onCompleteEditor()
{
    switchToIdle();
}

void WindowManager::captureFullScreens()
{
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

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, m_screenWidth, m_screenHeight);
    if (!hBitmap) {
        DeleteDC(hdcMemory);
        ReleaseDC(NULL, hdcScreen);
        qDebug() << "Failed to create bitmap";
        return;
    }

    HBITMAP hOldBitmap = (HBITMAP) SelectObject(hdcMemory, hBitmap);

    if (!BitBlt(hdcMemory,
                0,
                0,
                m_screenWidth,
                m_screenHeight,
                hdcScreen,
                m_screenLeft,
                m_screenTop,
                SRCCOPY)) {
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

    m_screenshotImg = QImage(bmpScreen.bmWidth, bmpScreen.bmHeight, QImage::Format_ARGB32);
    memcpy(m_screenshotImg.bits(), lpBits.get(), dataSize);
    m_screenshotPixmap = QPixmap::fromImage(m_screenshotImg);

    SelectObject(hdcMemory, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMemory);
    ReleaseDC(NULL, hdcScreen);
}

void WindowManager::switchToCapture()
{
    m_state = State::Capturing;
    m_screenshotWindow->showWindow();
}

void WindowManager::switchToEdit()
{
    m_state = State::Editing;
    m_screenshotWindow->hide();
}

void WindowManager::switchToIdle()
{
    m_state = State::Idle;
    if (m_screenshotWindow)
        m_screenshotWindow->hide();
    if (m_editorWindow)
        m_editorWindow->hideWindow();

    SetCursor(m_originalCursor);
}
