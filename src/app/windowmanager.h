#pragma once

#include <QObject>
#include <QPixmap>
#include <QPoint>

#include <memory>
#include <vector>

class PinWindow;
class SettingsWindow;

class WindowManager : public QObject
{
    Q_OBJECT

public:
    explicit WindowManager(QObject *parent = nullptr);
    ~WindowManager() override;

    void pinImage(const QPixmap &pixmap, const QPoint &position);

public slots:
    void onSettingActTriggered();
    void onAboutActTriggered();

private:
    void showSettingsWindow();

    std::unique_ptr<SettingsWindow> m_settingsWindow;
    std::vector<PinWindow *>        m_pinWindows;
};
