#ifndef OUTPUTSETTINGS_H
#define OUTPUTSETTINGS_H

#include <QDateTime>
#include <QLabel>
#include <QWidget>

namespace Ui {
class OutputSettings;
}

class OutputSettings : public QWidget
{
    Q_OBJECT

public:
    explicit OutputSettings(QWidget *parent = nullptr);
    ~OutputSettings();

private slots:
    void onManualFileNameChanged(const QString &text);
    void onQuickSavePathChanged(const QString &text);
    void onOpenFoldersClicked();
    void onChangeFoldersClicked();
    void onNamingRuleClicked();
    void onRestoreDefaultClicked();
    void onConfigChanged();

private:
    void    loadSettings();
    void    updatePreview(QLabel *previewLabel, const QString &pattern);
    QString generatePreviewText(const QString &pattern);

private:
    Ui::OutputSettings *ui;
};

#endif // OUTPUTSETTINGS_H
