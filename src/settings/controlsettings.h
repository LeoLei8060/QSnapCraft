#ifndef CONTROLSETTINGS_H
#define CONTROLSETTINGS_H

#include <windows.h>
#include <QWidget>

namespace Ui {
class ControlSettings;
}

class QHotKeyLineEdit;

class ControlSettings : public QWidget
{
    Q_OBJECT

public:
    explicit ControlSettings(QWidget *parent = nullptr);
    ~ControlSettings();

protected:
    void initializeConnect();

private slots:
    void onRestoreDefaultClicked();

private:
    void    loadSettings();
    QString getDefaultShortcutKey(int type) const;
    void    updateShortcutDisplay(QHotKeyLineEdit *lineEdit, const QString &key);
    QString keyEventToString(QKeyEvent *event) const;
    void    updateData(int id, const QString &val);

    // 快捷键解析相关函数
    static QStringList parseShortcutString(const QString &shortcut); // 拆分快捷键字符串
    static UINT        getVirtualKeyCode(const QString &keyStr);     // 获取虚拟键码

private:
    Ui::ControlSettings *ui;
};

#endif // CONTROLSETTINGS_H
