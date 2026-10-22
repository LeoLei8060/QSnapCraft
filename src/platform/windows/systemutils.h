#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <QString>

class SystemUtils
{
public:
    // 设置开机自启动
    static bool setAutoStart(bool enable);
    
    // 检查是否已设置开机自启动
    static bool isAutoStartEnabled();
};

#endif // SYSTEMUTILS_H
