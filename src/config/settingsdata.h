#ifndef SETTINGSDATA_H
#define SETTINGSDATA_H

#include "capture/captureoptions.h"
#include "pinoptions.h"

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
    QString captureHotkey = "F1";             // 截屏快捷键
    QString captureCopyHotkey = "Ctrl+F1";    // 截屏并自动复制
    QString captureFullDesktopHotkey = "F6";  // 全桌面截图
    QString captureLastAreaHotkey = "F7";     // 上次区域截图
    QString captureActiveScreenHotkey = "F8"; // 当前屏幕截图
    QString captureFixedSizeHotkey = "F9";    // 固定尺寸居中截图
    int     fixedCaptureWidth = 800;
    int     fixedCaptureHeight = 600;
    QString showLastPinHotkey = "F3";          // 显示最近贴图
    QString hidePinsHotkey = "F2";             // 隐藏全部可见贴图
    QString showHiddenPinsHotkey = "Shift+F2"; // 显示全部隐藏贴图
    QString showPinsHotkey = "Shift+F3";       // 兼容旧配置，不再注册
    QString clickThroughHotkey = "F4";         // 切换贴图鼠标穿透
    QString transparencyHotkey = "Ctrl+Wheel"; // 贴图透明度
    QString closePinHotkey = "F5";             // 关闭鼠标下贴图
    QString recoverClosedPinHotkey = "Shift+F5";
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
    PinSettingsData        pin;
    OcrSettingsData        ocr;
};

#endif // SETTINGSDATA_H
