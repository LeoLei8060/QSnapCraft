#ifndef SCREENSHOTSETTINGS_H
#define SCREENSHOTSETTINGS_H

#include <QWidget>

namespace Ui {
class ScreenshotSettings;
}

class ScreenshotSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenshotSettings(QWidget *parent = nullptr);
    ~ScreenshotSettings();

    void loadSettings();
    void saveSettings();

private:
    Ui::ScreenshotSettings *ui;
};

#endif // SCREENSHOTSETTINGS_H
