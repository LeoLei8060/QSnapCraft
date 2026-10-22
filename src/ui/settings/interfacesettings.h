#pragma once

#include "config/settingsdata.h"

#include <QWidget>

class QCheckBox;
class QSpinBox;

class InterfaceSettings : public QWidget
{
    Q_OBJECT

public:
    explicit InterfaceSettings(QWidget *parent = nullptr);
    ~InterfaceSettings() override;

    void setPreferences(const ApplicationPreferences &preferences);
    void writePreferences(ApplicationPreferences &preferences) const;
    void restoreDefaults();

signals:
    void sigChanged();

private:
    QCheckBox *m_enableCaptureBar{nullptr};
    QCheckBox *m_autoHide{nullptr};
    QSpinBox  *m_snapDistance{nullptr};
    QSpinBox  *m_hotspotWidth{nullptr};
    QSpinBox  *m_hotspotLength{nullptr};
    QSpinBox  *m_hideDelay{nullptr};
    QSpinBox  *m_revealDelay{nullptr};
    QSpinBox  *m_slideDuration{nullptr};
    bool       m_resetPlacementRequested{false};
};
