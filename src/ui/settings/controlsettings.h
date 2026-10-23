#pragma once

#include "config/settingsdata.h"

#include <QWidget>

class QHotKeyLineEdit;
class QLabel;
class QSpinBox;

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
    QWidget *createHotkeyRow(QHotKeyLineEdit *lineEdit, QLabel **status, const QString &statusObjectName);
    void     refreshHotkeyStatuses();

    QHotKeyLineEdit *m_captureHotkey{nullptr};
    QHotKeyLineEdit *m_captureCopyHotkey{nullptr};
    QHotKeyLineEdit *m_captureFullDesktopHotkey{nullptr};
    QHotKeyLineEdit *m_captureLastAreaHotkey{nullptr};
    QHotKeyLineEdit *m_captureActiveScreenHotkey{nullptr};
    QHotKeyLineEdit *m_captureFixedSizeHotkey{nullptr};
    QHotKeyLineEdit *m_clickThroughHotkey{nullptr};
    QSpinBox        *m_fixedWidth{nullptr};
    QSpinBox        *m_fixedHeight{nullptr};
    QLabel          *m_captureStatus{nullptr};
    QLabel          *m_captureCopyStatus{nullptr};
    QLabel          *m_captureFullDesktopStatus{nullptr};
    QLabel          *m_captureLastAreaStatus{nullptr};
    QLabel          *m_captureActiveScreenStatus{nullptr};
    QLabel          *m_captureFixedSizeStatus{nullptr};
    QLabel          *m_clickThroughStatus{nullptr};
};
