#ifndef GENERALSETTINGS_H
#define GENERALSETTINGS_H

#include <QWidget>

namespace Ui {
class GeneralSettings;
}

class GeneralSettings : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralSettings(QWidget *parent = nullptr);
    ~GeneralSettings();

    void loadSettings();
    void saveSettings();

private:
    Ui::GeneralSettings *ui;
};

#endif // GENERALSETTINGS_H
