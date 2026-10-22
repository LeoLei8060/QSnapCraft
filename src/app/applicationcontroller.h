#pragma once

#include "app/applicationaction.h"

#include <QObject>
#include <QRect>
#include <QTimer>

#include <memory>

class CaptureBar;
class CaptureSession;
class ColorPickerWindow;
class GlobalConfig;
class ScreenRulerWindow;
class SystemTray;
class WindowManager;

class ApplicationController : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;

    bool initialize();

public slots:
    void activate();
    void handleMessage(const QString &message);
    void startScreenshot(bool copy = false);
    void onActionTriggered(ApplicationAction action);

private:
    void initializeHotKey();
    void startCaptureWithSelection(bool copy, const QRect &selection);
    void applyCaptureBarSettings();
    void restoreCaptureBar();

    std::unique_ptr<GlobalConfig>      m_globalConfig;
    std::unique_ptr<SystemTray>        m_systemTray;
    std::unique_ptr<WindowManager>     m_windowManager;
    std::unique_ptr<CaptureBar>        m_captureBar;
    std::unique_ptr<ColorPickerWindow> m_colorPicker;
    std::unique_ptr<ScreenRulerWindow> m_screenRuler;
    std::unique_ptr<CaptureSession>    m_captureSession;
    QTimer                             m_foregroundTimer;
    quintptr                           m_lastExternalWindow{0};
    bool                               m_initialized{false};
};
