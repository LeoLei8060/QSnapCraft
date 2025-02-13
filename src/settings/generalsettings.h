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

private slots:
    void onAutoStartChanged(bool checked);
    void onOpenFolderClicked();
    void onOpenFileClicked();
    void onConfigChanged();

private:
    void loadSettings();
    void updateConfigPath(const QString &path);

private:
    Ui::GeneralSettings *ui;
};

#endif // GENERALSETTINGS_H
