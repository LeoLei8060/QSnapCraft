#include "WindowManager.h"
#include <QDebug>

WindowManager::WindowManager(QObject *parent)
    : QObject(parent)
{
    // 创建窗口实例
    m_screenshotWindow = std::make_unique<ScreenshotWindow>();
    m_editorWindow = std::make_unique<EditorWindow>();

    // 连接信号
    connect(m_screenshotWindow.get(),
            &ScreenshotWindow::sigStartEdit,
            this,
            &WindowManager::onScreenshotFinished);
    connect(m_editorWindow.get(), &QWidget::destroyed, this, &WindowManager::onEditorFinished);
}

WindowManager::~WindowManager() {}

void WindowManager::startCapture()
{
    if (m_state != State::Idle) {
        return;
    }
    switchToCapture();
}

void WindowManager::finishCapture()
{
    switchToIdle();
}

void WindowManager::startEdit()
{
    onScreenshotFinished();
}

void WindowManager::onScreenshotFinished()
{
    if (m_state != State::Capturing) {
        return;
    }

    // 获取截图数据
    QImage image = m_screenshotWindow->getCaptureImage();
    QRect  rect = m_screenshotWindow->getCaptureRect();

    // 切换到编辑状态
    switchToEdit();

    // 设置编辑窗口数据
    m_editorWindow->start(image, rect);
    m_editorWindow->show();
}

void WindowManager::onEditorFinished()
{
    switchToIdle();
}

void WindowManager::switchToCapture()
{
    m_state = State::Capturing;
    m_screenshotWindow->start();
}

void WindowManager::switchToEdit()
{
    m_state = State::Editing;
    m_screenshotWindow->hide();
}

void WindowManager::switchToIdle()
{
    m_state = State::Idle;
    if (m_screenshotWindow) {
        m_screenshotWindow->hide();
    }
    if (m_editorWindow) {
        m_editorWindow->hide();
    }
}
