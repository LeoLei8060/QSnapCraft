#pragma once

#include "config/settingsdata.h"

#include <QDialog>
#include <QSet>

class AboutSettings;
class ControlSettings;
class GeneralSettings;
class InterfaceSettings;
class OcrSettings;
class OutputSettings;
class PinSettings;
class QListWidget;
class QPushButton;
class QStackedWidget;
class ScreenshotSettings;

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow() override;

    void showPage(int index);
    void showAboutPage();

signals:
    void sigPreferencesApplied();

protected:
    void closeEvent(QCloseEvent *event) override;
    void reject() override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onPageChanged();
    void onApplyClicked();
    void onCancelClicked();
    void onOkClicked();
    void onRestorePageClicked();

private:
    void setupUi();
    void loadPreferences();
    void discardChanges();
    bool applyChanges();
    void updateDirtyState();
    int  pageIndex(const QWidget *page) const;
    void resetAllPages(const ApplicationPreferences &preferences);

    QListWidget           *m_pageList{nullptr};
    QStackedWidget        *m_pageStack{nullptr};
    QPushButton           *m_restorePageButton{nullptr};
    QPushButton           *m_applyButton{nullptr};
    QPushButton           *m_cancelButton{nullptr};
    QPushButton           *m_okButton{nullptr};
    GeneralSettings       *m_generalSettings{nullptr};
    ScreenshotSettings    *m_screenshotSettings{nullptr};
    OutputSettings        *m_outputSettings{nullptr};
    InterfaceSettings     *m_interfaceSettings{nullptr};
    PinSettings           *m_pinSettings{nullptr};
    OcrSettings           *m_ocrSettings{nullptr};
    ControlSettings       *m_controlSettings{nullptr};
    AboutSettings         *m_aboutSettings{nullptr};
    ApplicationPreferences m_initialPreferences;
    QSet<int>              m_dirtyPages;
};
