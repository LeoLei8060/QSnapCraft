#include "windowmanager.h"

#include "pinboard/pinwindow.h"
#include "ui/settings/settingswindow.h"

WindowManager::WindowManager(QObject *parent)
    : QObject(parent)
{}

WindowManager::~WindowManager() = default;

void WindowManager::onSettingActTriggered()
{
    showSettingsWindow();
    m_settingsWindow->showPage(0);
}

void WindowManager::onAboutActTriggered()
{
    showSettingsWindow();
    m_settingsWindow->showAboutPage();
}

void WindowManager::showSettingsWindow()
{
    if (!m_settingsWindow) {
        m_settingsWindow = std::make_unique<SettingsWindow>();
    }
}

void WindowManager::pinImage(const QPixmap &pixmap, const QPoint &position)
{
    if (pixmap.isNull()) {
        return;
    }
    auto *pinWindow = new PinWindow();
    pinWindow->pinImage(pixmap, QRect(position, pixmap.size()));
    m_pinWindows.push_back(pinWindow);
}
