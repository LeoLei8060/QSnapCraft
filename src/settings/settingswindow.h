#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include "aboutsettings.h"
#include "controlsettings.h"
#include "generalsettings.h"
#include "outputsettings.h"
#include "screenshotsettings.h"
#include <QWidget>

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow();

private:
    void setupUi();

private:
    Ui::SettingsWindow *ui;
    GeneralSettings    *generalSettings;
    ScreenshotSettings *screenshotSettings;
    OutputSettings     *outputSettings;
    ControlSettings    *controlSettings;
    AboutSettings      *aboutSettings;
};

#endif // SETTINGSWINDOW_H
