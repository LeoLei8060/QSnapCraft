#ifndef CONTROLSETTINGS_H
#define CONTROLSETTINGS_H

#include <QWidget>

namespace Ui {
class ControlSettings;
}

class ControlSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ControlSettings(QWidget *parent = nullptr);
    ~ControlSettings();

    void loadSettings();
    void saveSettings();

private:
    Ui::ControlSettings *ui;
};

#endif // CONTROLSETTINGS_H
