#pragma once

#include "config/settingsdata.h"

#include <QColor>
#include <QWidget>

class QCheckBox;
class QFontComboBox;
class QPushButton;
class QSpinBox;

class PinSettings : public QWidget
{
    Q_OBJECT

public:
    explicit PinSettings(QWidget *parent = nullptr);

    void setPreferences(const ApplicationPreferences &preferences);
    void writePreferences(ApplicationPreferences &preferences) const;
    void restoreDefaults();

signals:
    void sigChanged();

private:
    void chooseTextColor();
    void chooseBackgroundColor();
    void updateColorButtons();

    QCheckBox     *m_shadow{nullptr};
    QCheckBox     *m_stayOnTop{nullptr};
    QCheckBox     *m_smoothScaling{nullptr};
    QCheckBox     *m_zoomAtCursor{nullptr};
    QSpinBox      *m_recoveryLimit{nullptr};
    QSpinBox      *m_snapDistance{nullptr};
    QFontComboBox *m_fontFamily{nullptr};
    QSpinBox      *m_fontSize{nullptr};
    QSpinBox      *m_maxWidth{nullptr};
    QSpinBox      *m_padding{nullptr};
    QPushButton   *m_textColor{nullptr};
    QPushButton   *m_backgroundColor{nullptr};
    QColor         m_currentTextColor{Qt::black};
    QColor         m_currentBackgroundColor{Qt::white};
};
