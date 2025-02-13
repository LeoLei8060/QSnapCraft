#include "settingswindow.h"
#include "./ui_settingswindow.h"
#include "aboutsettings.h"
#include "controlsettings.h"
#include "generalsettings.h"
#include "outputsettings.h"
#include "screenshotsettings.h"
#include <QCloseEvent>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWindow)
    , m_generalSettings(new GeneralSettings(this))
    , m_screenshotSettings(new ScreenshotSettings(this))
    , m_outputSettings(new OutputSettings(this))
    , m_controlSettings(new ControlSettings(this))
    , m_aboutSettings(new AboutSettings(this))
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    setupUi();
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
    delete m_generalSettings;
    delete m_screenshotSettings;
    delete m_outputSettings;
    delete m_controlSettings;
    delete m_aboutSettings;
}

void SettingsWindow::setupUi()
{
    ui->tabWidget->addTab(m_generalSettings, tr("General"));
    ui->tabWidget->addTab(m_screenshotSettings, tr("Screenshot"));
    ui->tabWidget->addTab(m_outputSettings, tr("Output"));
    ui->tabWidget->addTab(m_controlSettings, tr("Control"));
    ui->tabWidget->addTab(m_aboutSettings, tr("About"));
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    hide();  // 隐藏窗口而不是关闭
    event->ignore();  // 忽略关闭事件
}
