#ifndef ABOUTSETTINGS_H
#define ABOUTSETTINGS_H

#include <QWidget>

namespace Ui {
class AboutSettings;
}

class AboutSettings : public QWidget
{
    Q_OBJECT

public:
    explicit AboutSettings(QWidget *parent = nullptr);
    ~AboutSettings();

private:
    Ui::AboutSettings *ui;
};

#endif // ABOUTSETTINGS_H
