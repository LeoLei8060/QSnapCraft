#pragma once

#include "editor/editorwindow.h"
#include "screenshot/screenshotwindow.h"
#include <memory>
#include <QObject>

class WindowManager : public QObject
{
    Q_OBJECT

public:
    // 截图工具状态
    enum class State {
        Idle,      // 空闲状态
        Capturing, // 正在截屏
        Editing    // 正在编辑
    };

    explicit WindowManager(QObject *parent = nullptr);
    ~WindowManager();

    void startCapture();  // 开始截图
    void finishCapture(); // 结束截图
    void startEdit();     // 开始编辑
    bool isCapturing() const { return m_state == State::Capturing; }

private slots:
    void onCancelScreenshot();   // 截图取消，返回空闲状态
    void onCompleteScreenshot(); // 截图完成，切换到编辑状态
    void onCancelEditor();       // 编辑取消，切换到截图状态
    void onCompleteEditor();     // 编辑完成，返回空闲状态

private:
    void switchToCapture(); // 切换到截图状态
    void switchToEdit();    // 切换到编辑状态
    void switchToIdle();    // 切换到空闲状态

    std::unique_ptr<ScreenshotWindow> m_screenshotWindow;
    std::unique_ptr<EditorWindow>     m_editorWindow;
    State                             m_state{State::Idle};

    HCURSOR m_originalCursor;
};
