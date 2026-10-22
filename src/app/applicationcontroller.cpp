#include "applicationcontroller.h"

#include "capture/capturesession.h"
#include "app/windowmanager.h"
#include "platform/windows/foregroundwindow.h"
#include "config/globalconfig.h"
#include "platform/windows/shortcutmanager.h"
#include "ui/systemtray.h"
#include "platform/windows/systemutils.h"
#include "ui/capturebar.h"
#include "ui/colorpickerwindow.h"
#include "ui/screenrulerwindow.h"

#include <QCoreApplication>
#include <QDebug>

#include <windows.h>

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
{}

ApplicationController::~ApplicationController() = default;

bool ApplicationController::initialize()
{
    if (m_initialized) {
        return true;
    }

    m_globalConfig = std::unique_ptr<GlobalConfig>(GlobalConfig::instance());
    SystemUtils::setAutoStart(GlobalConfig::instance()->generalData().autoStart);

    m_systemTray = std::make_unique<SystemTray>();
    m_windowManager = std::make_unique<WindowManager>();
    m_captureSession = std::make_unique<CaptureSession>();

    m_foregroundTimer.setInterval(250);
    connect(&m_foregroundTimer, &QTimer::timeout, this, [this]() {
        const HWND window = GetForegroundWindow();
        if (window == nullptr) {
            return;
        }
        DWORD process_id = 0;
        GetWindowThreadProcessId(window, &process_id);
        if (process_id != 0 && process_id != GetCurrentProcessId()) {
            m_lastExternalWindow = reinterpret_cast<quintptr>(window);
        }
    });
    m_foregroundTimer.start();

    connect(m_captureSession.get(), &CaptureSession::sigFinished, this, [this](const CaptureResult &result) {
        restoreCaptureBar();
        if (result.intent == CaptureIntent::kPin && !result.image.isNull()) {
            m_windowManager->pinImage(QPixmap::fromImage(result.image), result.globalPosition);
        }
    });
    connect(m_captureSession.get(), &CaptureSession::sigPinned, this, [this](const CaptureResult &result) {
        if (!result.image.isNull()) {
            m_windowManager->pinImage(QPixmap::fromImage(result.image), result.globalPosition);
        }
    });
    connect(m_captureSession.get(), &CaptureSession::sigCancelled, this, [this]() { restoreCaptureBar(); });
    connect(m_captureSession.get(), &CaptureSession::sigFailed, this, [this](const QString &error) {
        restoreCaptureBar();
        qWarning() << error;
    });

    m_captureBar = std::make_unique<CaptureBar>();
    connect(m_captureBar.get(), &CaptureBar::sigActionTriggered, this, &ApplicationController::onActionTriggered);
    connect(m_captureBar.get(),
            &CaptureBar::sigPlacementChanged,
            this,
            [](DockEdge       edge,
               qreal          position_ratio,
               const QString &screen_name,
               const QPoint  &free_position,
               bool           has_free_position,
               bool           horizontal) {
                auto data = GlobalConfig::instance()->captureBarData();
                data.dockEdge = dockEdgeName(edge);
                data.positionRatio = position_ratio;
                data.screenName = screen_name;
                data.freePosition = free_position;
                data.hasFreePosition = has_free_position;
                data.horizontal = horizontal;
                GlobalConfig::instance()->setCaptureBarData(data);
            });
    connect(GlobalConfig::instance(), &GlobalConfig::configChanged, this, [this]() {
        applyCaptureBarSettings();
        initializeHotKey();
        SystemUtils::setAutoStart(GlobalConfig::instance()->generalData().autoStart);
        if (m_systemTray) {
            m_systemTray->setCaptureBarVisible(GlobalConfig::instance()->captureBarData().enabled);
        }
    });
    applyCaptureBarSettings();
    initializeHotKey();

    connect(ShortcutManager::instance(), &ShortcutManager::hotkeyTriggered, this, [this](int id) {
        const auto key = static_cast<ShortcutManager::EKeyType>(id);
        switch (key) {
        case ShortcutManager::captureKey:
            startScreenshot();
            break;
        case ShortcutManager::captureCopyKey:
            startScreenshot(true);
            break;
        case ShortcutManager::EscapeKey:
            if (m_captureSession) {
                m_captureSession->cancel();
            }
            break;
        default:
            break;
        }
    });

    connect(m_systemTray.get(), &SystemTray::sigActionTriggered, this, &ApplicationController::onActionTriggered);
    m_systemTray->setCaptureBarVisible(GlobalConfig::instance()->captureBarData().enabled);
    m_systemTray->show();
    m_initialized = true;
    return true;
}

void ApplicationController::activate()
{
    if (m_captureSession && m_captureSession->isActive()) {
        return;
    }
    applyCaptureBarSettings();
    restoreCaptureBar();
}

void ApplicationController::handleMessage(const QString &message)
{
    if (message == QStringLiteral("quit")) {
        QTimer::singleShot(0, this, [this]() { onActionTriggered(ApplicationAction::kQuit); });
        return;
    }
    if (message == QStringLiteral("activate")) {
        activate();
    }
}

void ApplicationController::initializeHotKey()
{
    auto shortcut_manager = ShortcutManager::instance();
    const auto &control_data = GlobalConfig::instance()->controlData();
    shortcut_manager->unregisterAllHotkeys();

    if (!control_data.captureHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.captureHotkey, ShortcutManager::captureKey);
    }
    if (!control_data.captureCopyHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.captureCopyHotkey, ShortcutManager::captureCopyKey);
    }
    shortcut_manager->registerHotkey(QStringLiteral("Esc"), ShortcutManager::EscapeKey);
}

void ApplicationController::startScreenshot(bool copy)
{
    startCaptureWithSelection(copy, QRect());
}

void ApplicationController::startCaptureWithSelection(bool copy, const QRect &selection)
{
    if (!m_captureSession || m_captureSession->isActive()) {
        return;
    }

    if (m_screenRuler && m_screenRuler->isVisible()) {
        m_screenRuler->close();
    }
    if (m_captureBar) {
        m_captureBar->suppressForCapture();
    }
    QTimer::singleShot(50, this, [this, copy, selection]() {
        CaptureRequest request;
        request.intent = copy ? CaptureIntent::kCopy : CaptureIntent::kAnnotate;
        request.startMode = selection.isValid() ? CaptureStartMode::kTwoCorners : CaptureStartMode::kPoint;
        request.initialSelection = selection;
        m_captureSession->start(request, GlobalConfig::instance()->getPreferences().capture);
    });
}

void ApplicationController::onActionTriggered(ApplicationAction action)
{
    switch (action) {
    case ApplicationAction::kSnip:
        startScreenshot();
        break;
    case ApplicationAction::kCaptureActiveWindow: {
        HWND window = foregroundWindowExcludingCurrentProcess();
        if (window == nullptr && m_lastExternalWindow != 0) {
            HWND candidate = reinterpret_cast<HWND>(m_lastExternalWindow);
            if (IsWindow(candidate) && IsWindowVisible(candidate)) {
                window = candidate;
            }
        }
        const QRect bounds = windowFrameBounds(window);
        if (bounds.isValid()) {
            startCaptureWithSelection(false, bounds);
        } else {
            qWarning() << "No capturable foreground window";
        }
        break;
    }
    case ApplicationAction::kPickColor:
        if (m_captureSession && m_captureSession->isActive()) {
            break;
        }
        if (m_captureBar) {
            m_captureBar->suppressForCapture();
        }
        QTimer::singleShot(50, this, [this]() {
            if (!m_colorPicker) {
                m_colorPicker = std::make_unique<ColorPickerWindow>();
                connect(m_colorPicker.get(), &ColorPickerWindow::sigColorPicked, this, [this]() {
                    restoreCaptureBar();
                });
                connect(m_colorPicker.get(), &ColorPickerWindow::sigCancelled, this, [this]() {
                    restoreCaptureBar();
                });
            }
            m_colorPicker->start();
        });
        break;
    case ApplicationAction::kScreenRuler:
        if (m_captureSession && m_captureSession->isActive()) {
            break;
        }
        if (!m_screenRuler) {
            m_screenRuler = std::make_unique<ScreenRulerWindow>();
            connect(m_screenRuler.get(), &ScreenRulerWindow::sigClosed, this, [this]() {
                restoreCaptureBar();
            });
        }
        if (m_screenRuler->isMinimized()) {
            m_screenRuler->showNormal();
            m_screenRuler->raise();
        } else if (m_screenRuler->isVisible()) {
            m_screenRuler->close();
        } else {
            if (m_captureBar) {
                m_captureBar->suppressForCapture();
            }
            m_screenRuler->showAtCursor();
        }
        break;
    case ApplicationAction::kPreferences:
        m_windowManager->onSettingActTriggered();
        break;
    case ApplicationAction::kToggleCaptureBar: {
        auto data = GlobalConfig::instance()->captureBarData();
        data.enabled = !data.enabled;
        GlobalConfig::instance()->setCaptureBarData(data);
        break;
    }
    case ApplicationAction::kAbout:
        m_windowManager->onAboutActTriggered();
        break;
    case ApplicationAction::kQuit:
        if (m_systemTray) {
            m_systemTray->closeContextMenu();
        }
        QTimer::singleShot(0, qApp, []() { QCoreApplication::quit(); });
        break;
    default:
        break;
    }
}

void ApplicationController::applyCaptureBarSettings()
{
    if (!m_captureBar || !m_captureSession || m_captureSession->isActive()) {
        return;
    }

    const auto &data = GlobalConfig::instance()->captureBarData();
    m_captureBar->setSnapDistancePx(data.snapDistancePx);
    m_captureBar->setHotspotWidthPx(data.hotspotWidthPx);
    m_captureBar->setHotspotLengthPx(data.hotspotLengthPx);
    m_captureBar->setHideDelayMs(data.hideDelayMs);
    m_captureBar->setRevealDelayMs(data.revealDelayMs);
    m_captureBar->setSlideDurationMs(data.slideDurationMs);
    m_captureBar->setAutoHideEnabled(data.autoHide);

    if (!data.enabled) {
        m_captureBar->hideCompletely();
        return;
    }

    const DockEdge edge = dockEdgeFromName(data.dockEdge);
    m_captureBar->setFreeOrientation(data.horizontal);
    m_captureBar->showAtDock(edge,
                             data.positionRatio,
                             data.screenName,
                             data.freePosition,
                             edge == DockEdge::None && data.hasFreePosition);
}

void ApplicationController::restoreCaptureBar()
{
    if (m_captureBar) {
        m_captureBar->restoreAfterCapture();
    }
}
