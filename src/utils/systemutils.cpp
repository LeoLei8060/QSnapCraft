#include "systemutils.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>

bool SystemUtils::setAutoStart(bool enable)
{
#ifdef Q_OS_WIN
    QSettings bootSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                         QSettings::NativeFormat);
    if (enable) {
        QString appPath = QCoreApplication::applicationFilePath();
        bootSettings.setValue("QSnapCraft", QDir::toNativeSeparators(appPath));
    } else {
        bootSettings.remove("QSnapCraft");
    }
    return true;
#else
    // 其他系统的实现
    return false;
#endif
}

bool SystemUtils::isAutoStartEnabled()
{
#ifdef Q_OS_WIN
    QSettings bootSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                         QSettings::NativeFormat);
    return bootSettings.contains("QSnapCraft");
#else
    // 其他系统的实现
    return false;
#endif
}
