#pragma once

#include "config/settingsdata.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

class OutputSettings : public QWidget
{
    Q_OBJECT

public:
    explicit OutputSettings(QWidget *parent = nullptr);
    ~OutputSettings() override;

    void setPreferences(const ApplicationPreferences &preferences);
    void writePreferences(ApplicationPreferences &preferences) const;
    void restoreDefaults();

signals:
    void sigChanged();

private slots:
    void onBrowseManualPath();
    void onBrowseQuickPath();
    void onBrowseAutoPath();
    void onOpenFoldersClicked();
    void onNamingRuleClicked();

private:
    QString chooseDirectory(const QString &currentPath);
    void    updatePreview();
    void    updateQualityState();
    QString generatePreviewText(const QString &pattern) const;

    QLineEdit *m_manualPath{nullptr};
    QLineEdit *m_quickPath{nullptr};
    QLineEdit *m_autoPath{nullptr};
    QLineEdit *m_filenamePattern{nullptr};
    QComboBox *m_imageFormat{nullptr};
    QSpinBox  *m_jpegQuality{nullptr};
    QCheckBox *m_autoSave{nullptr};
    QCheckBox *m_soundEnabled{nullptr};
    QLabel    *m_preview{nullptr};
};
