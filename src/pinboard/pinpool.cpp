#include "pinpool.h"

#include "config/globalconfig.h"
#include "pinboard/pinwindow.h"
#include "pinboard/textimagerenderer.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QGuiApplication>
#include <QMimeData>
#include <QScreen>

#include <algorithm>

PinPool::PinPool(QObject *parent)
    : QObject(parent)
{
    connect(GlobalConfig::instance(), &GlobalConfig::configChanged, this, &PinPool::applyPinSettings);
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

void PinPool::pinImage(const QPixmap &pixmap, const QPoint &position)
{
    if (pixmap.isNull()) {
        return;
    }

    auto *pin_window = new PinWindow();
    connect(pin_window, &QObject::destroyed, this, [this]() { removeNullPins(); });
    connect(pin_window, &PinWindow::sigCloseRequested, this, &PinPool::removeClosedPin);
    const ApplicationPreferences preferences = GlobalConfig::instance()->getPreferences();
    pin_window->setShadowEnabled(preferences.pin.showShadow);
    pin_window->setStayOnTop(preferences.pin.stayOnTop);
    pin_window->pinImage(pixmap, QRect(position, pixmap.size()));
    m_pinWindows.push_back(pin_window);
}

int PinPool::pinCount() const
{
    return static_cast<int>(m_pinWindows.size());
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

    pinImage(pixmap, position);
    return true;
}

void PinPool::hideAllPins()
{
    removeNullPins();
    for (const QPointer<PinWindow> &pin_window : m_pinWindows) {
        if (pin_window != nullptr) {
            pin_window->hide();
        }
    }
}

void PinPool::showAllPins()
{
    removeNullPins();
    m_recentClosedPins.clear();
    for (const QPointer<PinWindow> &pin_window : m_pinWindows) {
        if (pin_window != nullptr) {
            pin_window->show();
            pin_window->raise();
        }
    }
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
        return true;
    }
    return false;
}

bool PinPool::toggleClickThroughAt(const QPoint &global_position)
{
    removeNullPins();
    for (auto it = m_pinWindows.rbegin(); it != m_pinWindows.rend(); ++it) {
        PinWindow *pin_window = *it;
        if (pin_window != nullptr && pin_window->isVisible() && pin_window->frameGeometry().contains(global_position)) {
            pin_window->setClickThrough(!pin_window->isClickThrough());
            return true;
        }
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

    QRect virtual_desktop;
    for (QScreen *screen : screens) {
        virtual_desktop = virtual_desktop.isNull() ? screen->geometry() : virtual_desktop.united(screen->geometry());
    }

    for (const QPointer<PinWindow> &pin_window : m_pinWindows) {
        if (pin_window == nullptr) {
            continue;
        }

        const QRect frame_rect = pin_window->frameGeometry();
        if (virtual_desktop.contains(frame_rect)) {
            continue;
        }

        QScreen *target_screen = QGuiApplication::screenAt(frame_rect.center());
        if (target_screen == nullptr) {
            int best_area = 0;
            for (QScreen *screen : screens) {
                const QRect intersection = frame_rect.intersected(screen->availableGeometry());
                const int   area = intersection.width() * intersection.height();
                if (area > best_area) {
                    best_area = area;
                    target_screen = screen;
                }
            }
        }
        if (target_screen == nullptr) {
            target_screen = QGuiApplication::primaryScreen();
        }
        if (target_screen == nullptr) {
            continue;
        }

        const QRect available = target_screen->availableGeometry();
        const int   max_x = qMax(available.left(), available.right() - frame_rect.width() + 1);
        const int   max_y = qMax(available.top(), available.bottom() - frame_rect.height() + 1);
        pin_window->move(qBound(available.left(), frame_rect.left(), max_x),
                         qBound(available.top(), frame_rect.top(), max_y));
    }
}

void PinPool::applyPinSettings()
{
    removeNullPins();
    const PinSettingsData settings = GlobalConfig::instance()->getPreferences().pin;
    for (const QPointer<PinWindow> &pin_window : m_pinWindows) {
        if (pin_window != nullptr) {
            pin_window->setShadowEnabled(settings.showShadow);
            pin_window->setStayOnTop(settings.stayOnTop);
        }
    }
}

void PinPool::removeClosedPin(PinWindow *window)
{
    const auto existing = std::find(m_recentClosedPins.begin(), m_recentClosedPins.end(), window);
    if (existing != m_recentClosedPins.end()) {
        m_recentClosedPins.erase(existing);
    }
    m_recentClosedPins.emplace_back(window);
}

void PinPool::removeNullPins()
{
    const auto is_null = [](const QPointer<PinWindow> &pin_window) { return pin_window.isNull(); };
    m_pinWindows.erase(std::remove_if(m_pinWindows.begin(), m_pinWindows.end(), is_null), m_pinWindows.end());
    m_recentClosedPins.erase(std::remove_if(m_recentClosedPins.begin(), m_recentClosedPins.end(), is_null),
                             m_recentClosedPins.end());
}
