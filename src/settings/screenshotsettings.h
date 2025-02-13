#ifndef SCREENSHOTSETTINGS_H
#define SCREENSHOTSETTINGS_H

#include <QWidget>

namespace Ui {
class ScreenshotSettings;
}

class ScreenshotSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenshotSettings(QWidget *parent = nullptr);
    ~ScreenshotSettings();

private slots:
    void onBorderWidthChanged(int value);
    void onLeftButtonActionChanged(int index);
    void onMiddleButtonActionChanged(int index);
    void onReturnActionChanged(int index);
    void onLeftButtonExitChanged(bool checked);
    void onMiddleButtonExitChanged(bool checked);
    void onReturnExitChanged(bool checked);
    void onRestoreDefaultClicked();
    void onConfigChanged();

private:
    void loadSettings();
    void updateComboBoxes();

private:
    Ui::ScreenshotSettings *ui;
};

#endif // SCREENSHOTSETTINGS_H
