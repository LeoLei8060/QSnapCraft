#include "aboutsettings.h"
#include "ui_about.h"

AboutSettings::AboutSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AboutSettings)
{
    ui->setupUi(this);
}

AboutSettings::~AboutSettings()
{
    delete ui;
}
