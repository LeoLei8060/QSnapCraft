#ifndef OUTPUTSETTINGS_H
#define OUTPUTSETTINGS_H

#include <QWidget>

namespace Ui {
class OutputSettings;
}

class OutputSettings : public QWidget
{
    Q_OBJECT

public:
    explicit OutputSettings(QWidget *parent = nullptr);
    ~OutputSettings();

    void loadSettings();
    void saveSettings();

private:
    Ui::OutputSettings *ui;
};

#endif // OUTPUTSETTINGS_H
