#include "generalsettings.h"
#include "ui_general.h"

GeneralSettings::GeneralSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GeneralSettings)
{
    ui->setupUi(this);
    loadSettings();
}

GeneralSettings::~GeneralSettings()
{
    delete ui;
}

void GeneralSettings::loadSettings()
{
    // TODO: Load settings from configuration
}

void GeneralSettings::saveSettings()
{
    // TODO: Save settings to configuration
}
