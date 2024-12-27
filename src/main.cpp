#include "core/screenshot/screenshot_window.h"
#include "utils/shortcut_manager.h"
#include "utils/system_tray.h"
#include <memory>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto systemTray = std::make_unique<SystemTray>();
    systemTray->show();

    auto shortcutManager = std::make_unique<ShortcutManager>();

    auto screenshotWindow = std::make_unique<ScreenshotWindow>();

    QObject::connect(shortcutManager.get(),
                     &ShortcutManager::screenshotTriggered,
                     screenshotWindow.get(),
                     &ScreenshotWindow::start);

    return app.exec();
}
