#pragma once

#include "editor/editorwindow.h"
#include "screenshot/screenshotwindow.h"
#include "settings/settingswindow.h"
#include <memory>
#include <vector>
#include <QObject>

class PinWindow;

class WindowManager : public QObject
{
    Q_OBJECT

public:
    // 截图工具状态
    enum class State {
        Idle,             // 空闲状态
        Capturing,        // 正在截屏
        CapturingAndCopy, // 正在截屏（截屏后自动复制）
        Editing           // 正在编辑
    };

    explicit WindowManager(QObject *parent = nullptr);
    ~WindowManager();

    void startCapture(bool bCopy = false); // 开始截图
    bool isCapturing() const { return m_state == State::Capturing; }

public slots:
    void onEscapePressed();

    void onSettingActTriggered();

private slots:
    void onCancelScreenshot();   // 截图取消，返回空闲状态
    void onCompleteScreenshot(); // 截图完成，切换到编辑状态
    void onCancelEditor();       // 编辑取消，切换到截图状态
    void onCompleteEditor();     // 编辑完成，返回空闲状态
    void onPinImage(const QPixmap &pixmap, const QRect &rect); // 将截图Pin到桌面上显示

private:
    void captureFullScreens();
    void switchToCapture(); // 切换到截图状态
    void switchToEdit();    // 切换到编辑状态
    void switchToIdle();    // 切换到空闲状态

    std::unique_ptr<ScreenshotWindow> m_screenshotWindow;
    std::unique_ptr<EditorWindow>     m_editorWindow;
    std::unique_ptr<SettingsWindow>   m_settingWindow;
    State                             m_state{State::Idle};

    int m_screenWidth{0};
    int m_screenHeight{0};
    int m_screenTop{0};
    int m_screenLeft{0};

    QPixmap m_screenshotPixmap;
    QImage  m_screenshotImg;

    std::vector<PinWindow *> m_pinWindows;

    HCURSOR m_originalCursor;
};
