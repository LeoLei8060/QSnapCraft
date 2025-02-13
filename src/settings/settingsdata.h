#ifndef SETTINGSDATA_H
#define SETTINGSDATA_H

#include <QString>

// 通用设置数据结构
struct GeneralSettingsData
{
    QString savePath = "config.ini"; // 配置文件保存路径（默认相对路径）
    bool    autoStart = true;        // 开机自启动
};

// 控制设置数据结构
struct ControlSettingsData
{
    QString captureHotkey = "F1";               // 截屏快捷键
    QString captureCopyHotkey = "Ctrl+F1";      // 截屏并自动复制
    QString showLastPinHotkey = "F3";           // 显示最近贴图
    QString showPinsHotkey = "Shift+F3";        // 隐藏/显示所有贴图
    QString transparencyHotkey = "Ctrl+Wheel";  // 贴图透明度
    QString closePinHotkey = "DoubleLeftClick"; // 关闭贴图
};

// 输出设置数据结构
struct OutputSettingsData
{
    QString fileNameFormat = "QSnapCraft_$yyyy-MM-dd_HH-mm-ss$.png"; // 手动保存文件名格式
    // NOTE: fastSavePath需要在软件初始化时自动设置（添加系统的桌面路径作为前缀）
    QString fastSavePath = "QSnapCraft_$yyyy-MM-dd_HH-mm-ss$.png"; // 快捷保存路径（带文件名格式）
    QString fastSaveHotkey = "Ctrl+Shift+S";                       // 快捷保存的快捷键
};

// 截图设置数据结构
struct ScreenshotSettingsData
{
    enum SS_Action {
        SS_None = 0,        // 无
        SS_CopytoClipboard, // 复制到剪贴板
        SS_PastetoScreen,   // 贴到屏幕
        SS_SavetoFile,      // 保存到文件
        SS_FastSave         // 快捷保存
    };

    uint borderWidth{2};            // 边框宽度
    int  doubleLeftAction{2};       // 左键双击的动作
    bool bExit_doubleLeft{true};    // 左键双击是否退出截屏
    int  doubleMiddleAction{0};     // 中键双击的动作
    bool bExit_doubleMiddle{false}; // 中键双击是否退出截屏
    int  enterAction{0};            // 回车键的动作
    bool bExit_enter{false};        // 回车键是否退出截屏
};

#endif // SETTINGSDATA_H
