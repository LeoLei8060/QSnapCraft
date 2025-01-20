#include "outputsettings.h"
#include "ui_output.h"

OutputSettings::OutputSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OutputSettings)
{
    ui->setupUi(this);
    loadSettings();
}

OutputSettings::~OutputSettings()
{
    delete ui;
}

void OutputSettings::loadSettings()
{
    // TODO: Load settings from configuration
}

void OutputSettings::saveSettings()
{
    // TODO: Save settings to configuration
}
