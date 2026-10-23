#include "pinpool.h"

#include "config/globalconfig.h"
#include "pinboard/pinimagelimits.h"
#include "pinboard/pinscreenfit.h"
#include "pinboard/pinsnapoverlay.h"
#include "pinboard/pinwindow.h"
#include "pinboard/textimagerenderer.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QGuiApplication>
#include <QMimeData>
#include <QScreen>
#include <QTimer>

#include <windows.h>

#include <algorithm>

PinPool::PinPool(QObject *parent)
    : QObject(parent)
{
    connect(GlobalConfig::instance(), &GlobalConfig::configChanged, this, &PinPool::applyPinSettings);
    applyPinSettings();
}

PinPool::~PinPool()
{
    while (!m_pinWindows.empty()) {
        QPointer<PinWindow> pin_window = m_pinWindows.back();
        m_pinWindows.pop_back();
        delete pin_window.data();
    }
    m_recentClosedPins.clear();
}

void PinPool::pinImage(const QPixmap &pixmap, const QPoint &position, const QSize &logicalSize)
{
    if (pixmap.isNull()) {
        return;
    }

    auto *pin_window = new PinWindow();
    connect(pin_window, &QObject::destroyed, this, [this]() { removeNullPins(); });
    connect(pin_window, &PinWindow::sigCloseRequested, this, &PinPool::removeClosedPin);
    connect(pin_window, &PinWindow::sigContentGeometryChanged, this, [this](PinWindow *, const QRect &) {
        refreshSnapTargets();
    });
    connect(pin_window, &PinWindow::sigSnapGuidesChanged, this, &PinPool::updateSnapOverlay);
    const ApplicationPreferences preferences = GlobalConfig::instance()->getPreferences();
    pin_window->setShadowEnabled(preferences.pin.showShadow);
    pin_window->setStayOnTop(preferences.pin.stayOnTop);
    pin_window->setSnapDistancePx(preferences.pin.snapDistancePx);
    pin_window->setSmoothScaling(preferences.pin.smoothScaling);
    pin_window->setZoomAtCursor(preferences.pin.zoomAtCursor);
    QSize       contentSize = logicalSize.isValid() ? logicalSize : pixmap.deviceIndependentSize().toSize();
    QPixmap     limitedPixmap = pixmap;
    const QSize boundedSize = boundedPinImageSize(pixmap.size());
    if (boundedSize != pixmap.size()) {
        if (!logicalSize.isValid()) {
            contentSize = boundedSize;
        }
        limitedPixmap = pixmap.scaled(boundedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        limitedPixmap.setDevicePixelRatio(pixmap.devicePixelRatio());
    }
    pin_window->pinImage(limitedPixmap, QRect(position, contentSize));
    m_pinWindows.push_back(pin_window);
    refreshSnapTargets();
}

int PinPool::pinCount() const
{
    return static_cast<int>(m_pinWindows.size());
}

void PinPool::setRecoveryLimit(int limit)
{
    m_recoveryLimit = qBound(0, limit, 50);
    pruneRecoveryPool();
}

bool PinPool::pasteFromClipboard()
{
    QImage image = QApplication::clipboard()->image();
    if (image.isNull()) {
        const QMimeData *mime_data = QApplication::clipboard()->mimeData();
        if (mime_data == nullptr) {
            return false;
        }

        const TextImageFormat format = mime_data->hasHtml() ? TextImageFormat::kHtml : TextImageFormat::kPlainText;
        const QString         content = mime_data->hasHtml() ? mime_data->html() : mime_data->text();
        image = renderTextToImage(content, format, GlobalConfig::instance()->getPreferences().pin.textToImage);
        if (image.isNull()) {
            return false;
        }
    }

    image = limitPinImageSize(image);
    if (image.isNull()) {
        return false;
    }
    const QPixmap pixmap = QPixmap::fromImage(image);
    const QSize   image_size = pixmap.deviceIndependentSize().toSize();
    QScreen      *screen = QGuiApplication::screenAt(QCursor::pos());
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return false;
    }
    const QRect  available = screen->availableGeometry();
    const QPoint cursor = QCursor::pos();
    const int    max_x = qMax(available.left(), available.right() - image_size.width() + 1);
    const int    max_y = qMax(available.top(), available.bottom() - image_size.height() + 1);
    const QPoint position(qBound(available.left(), cursor.x() + 16, max_x),
                          qBound(available.top(), cursor.y() + 16, max_y));

    pinImage(pixmap, position, image_size);
    return true;
}

void PinPool::hideAllPins()
{
    removeNullPins();
    if (m_snapOverlay) {
        m_snapOverlay->setGuides({});
    }
    for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
        if (pinWindow != nullptr && pinWindow->isVisible()) {
            pinWindow->hide();
            if (std::find(m_bulkHiddenPins.begin(), m_bulkHiddenPins.end(), pinWindow) == m_bulkHiddenPins.end()) {
                m_bulkHiddenPins.emplace_back(pinWindow);
            }
        }
    }
    refreshSnapTargets();
}

void PinPool::showAllPins()
{
    removeNullPins();
    for (const QPointer<PinWindow> &pinWindow : m_bulkHiddenPins) {
        if (pinWindow != nullptr) {
            pinWindow->show();
            pinWindow->raise();
        }
    }
    m_bulkHiddenPins.clear();
    refreshSnapTargets();
}

void PinPool::togglePinsVisibility()
{
    removeNullPins();
    const bool has_visible_pin = std::any_of(m_pinWindows.begin(), m_pinWindows.end(), [](const auto &pin_window) {
        return pin_window != nullptr && pin_window->isVisible();
    });
    if (has_visible_pin) {
        hideAllPins();
    } else {
        showAllPins();
    }
}

bool PinPool::recoverLastPin()
{
    removeNullPins();
    while (!m_recentClosedPins.empty()) {
        QPointer<PinWindow> pin_window = m_recentClosedPins.back();
        m_recentClosedPins.pop_back();
        if (pin_window == nullptr) {
            continue;
        }
        pin_window->show();
        pin_window->raise();
        pin_window->activateWindow();
        pin_window->setFocus();
        refreshSnapTargets();
        return true;
    }
    return false;
}

bool PinPool::toggleClickThroughAt(const QPoint &global_position)
{
    PinWindow *pinWindow = pinWindowAt(global_position);
    if (pinWindow != nullptr) {
        pinWindow->setClickThrough(!pinWindow->isClickThrough());
        return true;
    }
    return false;
}

void PinPool::ensurePinsVisible()
{
    removeNullPins();
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return;
    }

    QVector<QRect> availableScreens;
    for (QScreen *screen : screens) {
        availableScreens.append(screen->availableGeometry());
    }

    for (const QPointer<PinWindow> &pin_window : m_pinWindows) {
        if (pin_window == nullptr) {
            continue;
        }

        const QPoint position = clampPinToScreens(pin_window->frameGeometry(), availableScreens);
        pin_window->move(position);
    }
}

void PinPool::applyPinSettings()
{
    removeNullPins();
    const PinSettingsData settings = GlobalConfig::instance()->getPreferences().pin;
    setRecoveryLimit(settings.recoveryLimit);
    const bool shadowChanged = m_lastShowShadow != settings.showShadow;
    m_lastShowShadow = settings.showShadow;
    for (const QPointer<PinWindow> &pin_window : m_pinWindows) {
        if (pin_window != nullptr) {
            if (shadowChanged) {
                pin_window->setShadowEnabled(settings.showShadow);
            }
            pin_window->setStayOnTop(settings.stayOnTop);
            pin_window->setSnapDistancePx(settings.snapDistancePx);
            pin_window->setSmoothScaling(settings.smoothScaling);
            pin_window->setZoomAtCursor(settings.zoomAtCursor);
        }
    }
    refreshSnapTargets();
}

void PinPool::removeClosedPin(PinWindow *window)
{
    const auto existing = std::find(m_recentClosedPins.begin(), m_recentClosedPins.end(), window);
    if (existing != m_recentClosedPins.end()) {
        m_recentClosedPins.erase(existing);
    }
    if (m_snapOverlay) {
        m_snapOverlay->setGuides({});
    }
    const auto hidden = std::find(m_bulkHiddenPins.begin(), m_bulkHiddenPins.end(), window);
    if (hidden != m_bulkHiddenPins.end()) {
        m_bulkHiddenPins.erase(hidden);
    }
    if (m_recoveryLimit == 0) {
        const auto active = std::find(m_pinWindows.begin(), m_pinWindows.end(), window);
        if (active != m_pinWindows.end()) {
            m_pinWindows.erase(active);
        }
        window->deleteLater();
        QTimer::singleShot(0, this, [this]() { refreshSnapTargets(); });
        return;
    }
    m_recentClosedPins.emplace_back(window);
    pruneRecoveryPool();
    QTimer::singleShot(0, this, [this]() { refreshSnapTargets(); });
}

void PinPool::removeNullPins()
{
    const auto is_null = [](const QPointer<PinWindow> &pin_window) { return pin_window.isNull(); };
    m_pinWindows.erase(std::remove_if(m_pinWindows.begin(), m_pinWindows.end(), is_null), m_pinWindows.end());
    m_recentClosedPins.erase(std::remove_if(m_recentClosedPins.begin(), m_recentClosedPins.end(), is_null),
                             m_recentClosedPins.end());
    m_bulkHiddenPins.erase(std::remove_if(m_bulkHiddenPins.begin(), m_bulkHiddenPins.end(), is_null),
                           m_bulkHiddenPins.end());
}

void PinPool::pruneRecoveryPool()
{
    while (static_cast<int>(m_recentClosedPins.size()) > m_recoveryLimit) {
        QPointer<PinWindow> oldest = m_recentClosedPins.front();
        m_recentClosedPins.erase(m_recentClosedPins.begin());
        if (oldest == nullptr) {
            continue;
        }
        const auto active = std::find(m_pinWindows.begin(), m_pinWindows.end(), oldest);
        if (active != m_pinWindows.end()) {
            m_pinWindows.erase(active);
        }
        delete oldest.data();
    }
}

void PinPool::refreshSnapTargets()
{
    removeNullPins();
    for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
        if (pinWindow == nullptr) {
            continue;
        }
        if (!pinWindow->isVisible()) {
            pinWindow->setSnapTargets({});
            continue;
        }
        QVector<QRect> targets;
        for (const QPointer<PinWindow> &other : m_pinWindows) {
            if (other != nullptr && other != pinWindow && other->isVisible()) {
                targets.append(other->contentRect());
            }
        }
        pinWindow->setSnapTargets(targets);
    }
}

void PinPool::updateSnapOverlay(PinWindow *window, const QVector<QLineF> &guides)
{
    Q_UNUSED(window)
    if (guides.isEmpty()) {
        if (m_snapOverlay) {
            m_snapOverlay->setGuides({});
        }
        return;
    }
    if (!m_snapOverlay) {
        m_snapOverlay = std::make_unique<PinSnapOverlay>();
    }
    m_snapOverlay->setDesktopGeometry(virtualDesktopGeometry());
    m_snapOverlay->setGuides(guides);
}

PinWindow *PinPool::pinWindowAt(const QPoint &globalPosition)
{
    removeNullPins();

#ifdef Q_OS_WIN
    POINT point{globalPosition.x(), globalPosition.y()};
    HWND  underCursor = WindowFromPoint(point);
    if (underCursor != nullptr) {
        underCursor = GetAncestor(underCursor, GA_ROOT);
        for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
            if (pinWindow != nullptr && pinWindow->isVisible()
                && reinterpret_cast<HWND>(pinWindow->winId()) == underCursor) {
                return pinWindow;
            }
        }
    }

    for (HWND current = GetTopWindow(nullptr); current != nullptr; current = GetWindow(current, GW_HWNDNEXT)) {
        for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
            if (pinWindow != nullptr && pinWindow->isVisible() && reinterpret_cast<HWND>(pinWindow->winId()) == current
                && pinWindow->frameGeometry().contains(globalPosition)) {
                return pinWindow;
            }
        }
    }
#endif

    for (auto it = m_pinWindows.rbegin(); it != m_pinWindows.rend(); ++it) {
        PinWindow *pinWindow = *it;
        if (pinWindow != nullptr && pinWindow->isVisible() && pinWindow->frameGeometry().contains(globalPosition)) {
            return pinWindow;
        }
    }
    return nullptr;
}

QRect PinPool::virtualDesktopGeometry() const
{
    QRect virtualDesktop;
    for (QScreen *screen : QGuiApplication::screens()) {
        virtualDesktop = virtualDesktop.isNull() ? screen->geometry() : virtualDesktop.united(screen->geometry());
    }
    return virtualDesktop;
}
