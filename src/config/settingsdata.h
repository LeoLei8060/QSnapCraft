#ifndef SETTINGSDATA_H
#define SETTINGSDATA_H

#include "capture/captureoptions.h"

#include <QPoint>
#include <QString>

// 通用设置数据结构
struct GeneralSettingsData
{
    bool autoStart = true; // 开机自启动
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

// 桌面功能工具条设置
struct CaptureBarSettingsData
{
    bool    enabled{true};
    QString dockEdge{"Right"};
    QString screenName;
    qreal   positionRatio{0.5};
    bool    autoHide{true};
    int     snapDistancePx{12};
    int     hotspotWidthPx{4};
    int     hotspotLengthPx{50};
    int     hideDelayMs{850};
    int     revealDelayMs{200};
    int     slideDurationMs{300};
    QPoint  freePosition;
    bool    hasFreePosition{false};
    bool    horizontal{false};
};

struct ApplicationPreferences
{
    GeneralSettingsData    general;
    ControlSettingsData    control;
    CaptureBarSettingsData captureBar;
    CaptureOptions         capture;
};

#endif // SETTINGSDATA_H
