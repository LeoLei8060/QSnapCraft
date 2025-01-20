#include "settingswindow.h"
#include "./ui_settingswindow.h"
#include "aboutsettings.h"
#include "controlsettings.h"
#include "generalsettings.h"
#include "outputsettings.h"
#include "screenshotsettings.h"

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWindow)
    , generalSettings(new GeneralSettings(this))
    , screenshotSettings(new ScreenshotSettings(this))
    , outputSettings(new OutputSettings(this))
    , controlSettings(new ControlSettings(this))
    , aboutSettings(new AboutSettings(this))
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);

    setupUi();
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
    delete generalSettings;
    delete screenshotSettings;
    delete outputSettings;
    delete controlSettings;
    delete aboutSettings;
}

void SettingsWindow::setupUi()
{
    ui->tabWidget->addTab(generalSettings, tr("General"));
    ui->tabWidget->addTab(screenshotSettings, tr("Screenshot"));
    ui->tabWidget->addTab(outputSettings, tr("Output"));
    ui->tabWidget->addTab(controlSettings, tr("Control"));
    ui->tabWidget->addTab(aboutSettings, tr("About"));
}
