#include "windowmanager.h"

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
