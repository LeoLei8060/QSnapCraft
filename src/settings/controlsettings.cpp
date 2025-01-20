#include "controlsettings.h"
#include "ui_control.h"

ControlSettings::ControlSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ControlSettings)
{
    ui->setupUi(this);
    loadSettings();
}

ControlSettings::~ControlSettings()
{
    delete ui;
}

void ControlSettings::loadSettings()
{
    // TODO: Load settings from configuration
}

void ControlSettings::saveSettings()
{
    // TODO: Save settings to configuration
}
