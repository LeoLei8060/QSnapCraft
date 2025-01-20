#include "screenshotsettings.h"
#include "ui_screenshot.h"

ScreenshotSettings::ScreenshotSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScreenshotSettings)
{
    ui->setupUi(this);
    loadSettings();
}

ScreenshotSettings::~ScreenshotSettings()
{
    delete ui;
}

void ScreenshotSettings::loadSettings()
{
    // TODO: Load settings from configuration
}

void ScreenshotSettings::saveSettings()
{
    // TODO: Save settings to configuration
}
