#include "applicationcontroller.h"

#include "app/windowmanager.h"
#include "capture/capturesession.h"
#include "config/globalconfig.h"
#include "pinboard/pinpool.h"
#include "platform/windows/foregroundwindow.h"
#include "platform/windows/ocrengine.h"
#include "platform/windows/shortcutmanager.h"
#include "platform/windows/systemutils.h"
#include "ui/capturebar.h"
#include "ui/colorpickerwindow.h"
#include "ui/screenrulerwindow.h"
#include "ui/systemtray.h"

#include <QCoreApplication>
#include <QCursor>
#include <QDebug>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>
#include <QStandardPaths>

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
    connect(ShortcutManager::instance(),
            &ShortcutManager::sigRegistrationFailed,
            this,
            [this](int, const QString &, const QString &error) {
                if (m_systemTray) {
                    m_systemTray->showMessage(QStringLiteral("热键注册失败"), error);
                }
            });
    m_windowManager = std::make_unique<WindowManager>();
    const QString appDataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString pinStorageDirectory = appDataDirectory.isEmpty() ? QString()
                                                                   : appDataDirectory + QStringLiteral("/pins");
    m_pinPool = std::make_unique<PinPool>(pinStorageDirectory);
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
            m_pinPool->pinImage(QPixmap::fromImage(result.image), result.globalPosition, result.logicalSelection.size());
        }
    });
    connect(m_captureSession.get(), &CaptureSession::sigPinned, this, [this](const CaptureResult &result) {
        if (!result.image.isNull()) {
            m_pinPool->pinImage(QPixmap::fromImage(result.image), result.globalPosition, result.logicalSelection.size());
        }
    });
    connect(m_captureSession.get(), &CaptureSession::sigCancelled, this, [this]() { restoreCaptureBar(); });
    connect(m_captureSession.get(), &CaptureSession::sigFailed, this, [this](const QString &error) {
        restoreCaptureBar();
        qWarning() << error;
        QMessageBox::warning(nullptr, QStringLiteral("QSnapCraft"), error);
    });
    connect(m_captureSession.get(), &CaptureSession::sigOperationFailed, this, [this](const QString &error) {
        qWarning() << error;
        QMessageBox::warning(nullptr, QStringLiteral("QSnapCraft"), error);
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
        refreshOcrAvailability();
        setHotkeysEnabled(m_hotkeysEnabled);
        SystemUtils::setAutoStart(GlobalConfig::instance()->generalData().autoStart);
        if (m_systemTray) {
            m_systemTray->setCaptureBarVisible(GlobalConfig::instance()->captureBarData().enabled);
        }
    });
    applyCaptureBarSettings();
    refreshOcrAvailability();
    initializeHotKey();
    for (QScreen *screen : QGuiApplication::screens()) {
        watchScreen(screen);
    }
    connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        watchScreen(screen);
        refreshScreenTopology();
    });
    connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *) { refreshScreenTopology(); });
    connect(qApp, &QGuiApplication::primaryScreenChanged, this, [this](QScreen *) { refreshScreenTopology(); });
    m_systemTray->setHotkeysEnabled(m_hotkeysEnabled);

    connect(ShortcutManager::instance(), &ShortcutManager::hotkeyTriggered, this, [this](int id) {
        const auto key = static_cast<ShortcutManager::EKeyType>(id);
        switch (key) {
        case ShortcutManager::captureKey:
            startScreenshot();
            break;
        case ShortcutManager::captureCopyKey:
            startScreenshot(true);
            break;
        case ShortcutManager::captureFullDesktopKey:
            startScreenshotWithMode(CaptureStartMode::kFullDesktop);
            break;
        case ShortcutManager::captureLastAreaKey:
            startScreenshotWithMode(CaptureStartMode::kLastArea);
            break;
        case ShortcutManager::captureActiveScreenKey:
            startScreenshotWithMode(CaptureStartMode::kActiveScreen);
            break;
        case ShortcutManager::captureFixedSizeKey: {
            const auto &control = GlobalConfig::instance()->controlData();
            startScreenshotWithMode(CaptureStartMode::kCenteredSize,
                                    QSize(control.fixedCaptureWidth, control.fixedCaptureHeight));
            break;
        }
        case ShortcutManager::showLastPinKey:
            if (!m_pinPool->pasteFromClipboard()) {
                m_systemTray->showMessage(QStringLiteral("无法贴图"),
                                          QStringLiteral("剪贴板中没有可贴图的图像或文本内容。"));
            }
            break;
        case ShortcutManager::showPinsKey:
            m_pinPool->togglePinsVisibility();
            break;
        case ShortcutManager::hidePinsKey:
            m_pinPool->hideAllPins();
            break;
        case ShortcutManager::showHiddenPinsKey:
            m_pinPool->showAllPins();
            break;
        case ShortcutManager::closePinKey:
            m_pinPool->closePinAt(QCursor::pos());
            break;
        case ShortcutManager::recoverClosedPinKey:
            m_pinPool->recoverLastPin();
            break;
        case ShortcutManager::clickThroughKey:
            m_pinPool->toggleClickThroughAt(QCursor::pos());
            break;
        case ShortcutManager::scrollFinishKey:
            if (m_captureSession) {
                m_captureSession->finishScrollCapture();
            }
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
    auto        shortcut_manager = ShortcutManager::instance();
    const auto &control_data = GlobalConfig::instance()->controlData();
    shortcut_manager->unregisterAllHotkeys();

    if (!control_data.captureHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.captureHotkey, ShortcutManager::captureKey);
    }
    if (!control_data.captureCopyHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.captureCopyHotkey, ShortcutManager::captureCopyKey);
    }
    if (!control_data.captureFullDesktopHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.captureFullDesktopHotkey, ShortcutManager::captureFullDesktopKey);
    }
    if (!control_data.captureLastAreaHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.captureLastAreaHotkey, ShortcutManager::captureLastAreaKey);
    }
    if (!control_data.captureActiveScreenHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.captureActiveScreenHotkey,
                                         ShortcutManager::captureActiveScreenKey);
    }
    if (!control_data.captureFixedSizeHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.captureFixedSizeHotkey, ShortcutManager::captureFixedSizeKey);
    }
    if (!control_data.showLastPinHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.showLastPinHotkey, ShortcutManager::showLastPinKey);
    }
    if (!control_data.hidePinsHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.hidePinsHotkey, ShortcutManager::hidePinsKey);
    }
    if (!control_data.showHiddenPinsHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.showHiddenPinsHotkey, ShortcutManager::showHiddenPinsKey);
    }
    if (!control_data.closePinHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.closePinHotkey, ShortcutManager::closePinKey);
    }
    if (!control_data.recoverClosedPinHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.recoverClosedPinHotkey, ShortcutManager::recoverClosedPinKey);
    }
    if (!control_data.clickThroughHotkey.isEmpty()) {
        shortcut_manager->registerHotkey(control_data.clickThroughHotkey, ShortcutManager::clickThroughKey);
    }
    shortcut_manager->registerHotkey(QStringLiteral("Esc"), ShortcutManager::EscapeKey);
    m_hotkeysEnabled = true;
    if (m_systemTray) {
        m_systemTray->setHotkeysEnabled(true);
    }
}

void ApplicationController::setHotkeysEnabled(bool enabled)
{
    if (enabled) {
        initializeHotKey();
        return;
    }

    ShortcutManager::instance()->unregisterAllHotkeys();
    m_hotkeysEnabled = false;
    if (m_systemTray) {
        m_systemTray->setHotkeysEnabled(false);
    }
}

void ApplicationController::watchScreen(QScreen *screen)
{
    if (screen == nullptr) {
        return;
    }
    connect(screen, &QScreen::geometryChanged, this, &ApplicationController::refreshScreenTopology, Qt::UniqueConnection);
    connect(screen,
            &QScreen::availableGeometryChanged,
            this,
            &ApplicationController::refreshScreenTopology,
            Qt::UniqueConnection);
    connect(screen,
            &QScreen::logicalDotsPerInchChanged,
            this,
            &ApplicationController::refreshScreenTopology,
            Qt::UniqueConnection);
    connect(screen,
            &QScreen::physicalDotsPerInchChanged,
            this,
            &ApplicationController::refreshScreenTopology,
            Qt::UniqueConnection);
}

void ApplicationController::refreshScreenTopology()
{
    applyCaptureBarSettings();
    if (m_windowManager) {
        m_pinPool->ensurePinsVisible();
    }
}

void ApplicationController::startScreenshot(bool copy)
{
    startCaptureWithSelection(copy, QRect());
}

void ApplicationController::startScrollScreenshot()
{
    startCapture(CaptureIntent::kScroll, CaptureStartMode::kPoint);
}

void ApplicationController::startCaptureWithSelection(bool copy, const QRect &selection)
{
    startCapture(copy ? CaptureIntent::kCopy : CaptureIntent::kAnnotate,
                 selection.isValid() ? CaptureStartMode::kTwoCorners : CaptureStartMode::kPoint,
                 selection);
}

void ApplicationController::startScreenshotWithMode(CaptureStartMode mode, const QSize &centered_size)
{
    startCapture(CaptureIntent::kAnnotate, mode, {}, centered_size);
}

void ApplicationController::startCapture(CaptureIntent    intent,
                                         CaptureStartMode start_mode,
                                         const QRect     &selection,
                                         const QSize     &centered_size)
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
    QTimer::singleShot(50, this, [this, intent, start_mode, selection, centered_size]() {
        CaptureRequest request;
        request.intent = intent;
        request.startMode = start_mode;
        request.initialSelection = selection;
        request.centeredSize = centered_size;
        const ApplicationPreferences preferences = GlobalConfig::instance()->getPreferences();
        CaptureOptions               options = preferences.capture;
        options.ocrLanguage = preferences.ocr.language;
        m_captureSession->start(request, options);
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
    case ApplicationAction::kScrollSnip:
        startScrollScreenshot();
        break;
    case ApplicationAction::kOcrSnip:
        if (!isOcrEngineAvailable()) {
            refreshOcrAvailability();
            m_systemTray->showMessage(QStringLiteral("OCR 不可用"),
                                      QStringLiteral("Windows 未安装可用的 OCR 语言包，请在首选项的 OCR 页面查看"
                                                     "已识别语言并打开系统语言设置。"));
            break;
        }
        startCapture(CaptureIntent::kOcr, CaptureStartMode::kPoint);
        break;
    case ApplicationAction::kPaste:
        if (!m_pinPool->pasteFromClipboard()) {
            m_systemTray->showMessage(QStringLiteral("无法贴图"),
                                      QStringLiteral("剪贴板中没有可贴图的图像或文本内容。"));
        }
        break;
    case ApplicationAction::kHideAllImages:
        m_pinPool->hideAllPins();
        break;
    case ApplicationAction::kShowAllImages:
        m_pinPool->showAllPins();
        break;
    case ApplicationAction::kRecoverImage:
        if (!m_pinPool->recoverLastPin()) {
            qWarning() << "There is no closed pin to restore";
        }
        break;
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
                connect(m_colorPicker.get(), &ColorPickerWindow::sigCancelled, this, [this]() { restoreCaptureBar(); });
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
            connect(m_screenRuler.get(), &ScreenRulerWindow::sigClosed, this, [this]() { restoreCaptureBar(); });
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
    case ApplicationAction::kToggleHotkeys:
        setHotkeysEnabled(!m_hotkeysEnabled);
        break;
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

void ApplicationController::refreshOcrAvailability()
{
    const bool available = isOcrEngineAvailable();
    if (m_captureBar) {
        m_captureBar->setOcrAvailable(available);
    }
    if (m_systemTray) {
        m_systemTray->setOcrAvailable(available);
    }
}

void ApplicationController::restoreCaptureBar()
{
    if (m_captureBar) {
        m_captureBar->restoreAfterCapture();
    }
}
