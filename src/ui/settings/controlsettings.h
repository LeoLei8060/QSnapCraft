#pragma once

#include "config/settingsdata.h"

#include <QWidget>

class QHotKeyLineEdit;

class ControlSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ControlSettings(QWidget *parent = nullptr);
    ~ControlSettings() override;

    void setPreferences(const ApplicationPreferences &preferences);
    void writePreferences(ApplicationPreferences &preferences) const;
    void restoreDefaults();

signals:
    void sigChanged();

private:
    QHotKeyLineEdit *m_captureHotkey{nullptr};
    QHotKeyLineEdit *m_captureCopyHotkey{nullptr};
};
