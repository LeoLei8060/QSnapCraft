#pragma once

#include "config/settingsdata.h"

#include <QColor>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QPushButton;
class QSpinBox;

class ScreenshotSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenshotSettings(QWidget *parent = nullptr);
    ~ScreenshotSettings() override;

    void setPreferences(const ApplicationPreferences &preferences);
    void writePreferences(ApplicationPreferences &preferences) const;
    void restoreDefaults();

signals:
    void sigChanged();

private slots:
    void onChooseAnchorColor();

private:
    QComboBox *createOutputActionCombo(const QString &objectName);
    void       setOutputGesture(
              QComboBox *actionCombo, QCheckBox *exitCheck, bool enabled, CaptureOutputAction action, bool exit);
    void updateMagnifierState();
    void updateColorButton();

    QCheckBox   *m_detectWindows{nullptr};
    QCheckBox   *m_detectUiElements{nullptr};
    QCheckBox   *m_showAnchors{nullptr};
    QCheckBox   *m_showTips{nullptr};
    QCheckBox   *m_autoCrossLines{nullptr};
    QCheckBox   *m_captureCursor{nullptr};
    QCheckBox   *m_neverCaptureCursor{nullptr};
    QCheckBox   *m_disableMouseScroll{nullptr};
    QCheckBox   *m_uppercaseHexColor{nullptr};
    QCheckBox   *m_physicalSelectionUnits{nullptr};
    QCheckBox   *m_showMagnifier{nullptr};
    QCheckBox   *m_magnifierMask{nullptr};
    QCheckBox   *m_magnifierBorder{nullptr};
    QCheckBox   *m_magnifierAnchors{nullptr};
    QCheckBox   *m_confirmOnEscape{nullptr};
    QCheckBox   *m_abortOnFocusOut{nullptr};
    QCheckBox   *m_hide1x1{nullptr};
    QCheckBox   *m_drawOutsideArea{nullptr};
    QCheckBox   *m_loopHistory{nullptr};
    QSpinBox    *m_maxHistoryAreas{nullptr};
    QSpinBox    *m_borderWidth{nullptr};
    QPushButton *m_anchorColorButton{nullptr};
    QComboBox   *m_enterAction{nullptr};
    QComboBox   *m_doubleClickAction{nullptr};
    QComboBox   *m_middleClickAction{nullptr};
    QCheckBox   *m_enterExit{nullptr};
    QCheckBox   *m_doubleClickExit{nullptr};
    QCheckBox   *m_middleClickExit{nullptr};
    QColor       m_anchorColor;
};
