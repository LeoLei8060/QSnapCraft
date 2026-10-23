#pragma once

#include "config/settingsdata.h"

#include <QWidget>

class QComboBox;

class OcrSettings : public QWidget
{
    Q_OBJECT

public:
    explicit OcrSettings(QWidget *parent = nullptr);

    void setPreferences(const ApplicationPreferences &preferences);
    void writePreferences(ApplicationPreferences &preferences) const;
    void restoreDefaults();

signals:
    void sigChanged();

private:
    QComboBox *m_language{nullptr};
};
