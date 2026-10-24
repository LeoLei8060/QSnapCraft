#pragma once

#include "app/applicationaction.h"
#include "capture/capturetypes.h"

#include <QObject>
#include <QRect>
#include <QSize>
#include <QTimer>

#include <memory>

class CaptureBar;
class CaptureSession;
class ColorPickerWindow;
class GlobalConfig;
class PinPool;
class QScreen;
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
    void setHotkeysEnabled(bool enabled);
    void watchScreen(QScreen *screen);
    void refreshScreenTopology();
    void startCapture(CaptureIntent    intent,
                      CaptureStartMode start_mode,
                      const QRect     &selection = {},
                      const QSize     &centered_size = {});
    void startCaptureWithSelection(bool copy, const QRect &selection);
    void startScrollScreenshot();
    void startScreenshotWithMode(CaptureStartMode mode, const QSize &centered_size = {});
    void applyCaptureBarSettings();
    void refreshOcrAvailability();
    void restoreCaptureBar();

    std::unique_ptr<GlobalConfig>      m_globalConfig;
    std::unique_ptr<SystemTray>        m_systemTray;
    std::unique_ptr<WindowManager>     m_windowManager;
    std::unique_ptr<PinPool>           m_pinPool;
    std::unique_ptr<CaptureBar>        m_captureBar;
    std::unique_ptr<ColorPickerWindow> m_colorPicker;
    std::unique_ptr<ScreenRulerWindow> m_screenRuler;
    std::unique_ptr<CaptureSession>    m_captureSession;
    QTimer                             m_foregroundTimer;
    quintptr                           m_lastExternalWindow{0};
    bool                               m_initialized{false};
    bool                               m_hotkeysEnabled{true};
};
