#include "applicationaction.h"

QVector<ApplicationAction> captureBarActions()
{
    return {
        ApplicationAction::kSnip,
        ApplicationAction::kCaptureActiveWindow,
        ApplicationAction::kScrollSnip,
        ApplicationAction::kOcrSnip,
        ApplicationAction::kPickColor,
        ApplicationAction::kScreenRuler,
        ApplicationAction::kPreferences,
    };
}

QString actionDisplayName(ApplicationAction action)
{
    switch (action) {
    case ApplicationAction::kSnip:
        return QStringLiteral("自动捕获");
    case ApplicationAction::kCaptureActiveWindow:
        return QStringLiteral("捕获活动窗口");
    case ApplicationAction::kScrollSnip:
        return QStringLiteral("滚动截图");
    case ApplicationAction::kOcrSnip:
        return QStringLiteral("OCR");
    case ApplicationAction::kPaste:
        return QStringLiteral("贴图");
    case ApplicationAction::kPickColor:
        return QStringLiteral("屏幕取色");
    case ApplicationAction::kScreenRuler:
        return QStringLiteral("屏幕标尺");
    case ApplicationAction::kToggleCaptureBar:
        return QStringLiteral("显示桌面工具条");
    case ApplicationAction::kHideAllImages:
        return QStringLiteral("隐藏全部贴图");
    case ApplicationAction::kShowAllImages:
        return QStringLiteral("显示全部贴图");
    case ApplicationAction::kRecoverImage:
        return QStringLiteral("恢复贴图");
    case ApplicationAction::kPreferences:
        return QStringLiteral("首选项");
    case ApplicationAction::kToggleHotkeys:
        return QStringLiteral("启用或禁用热键");
    case ApplicationAction::kHelp:
        return QStringLiteral("帮助");
    case ApplicationAction::kAbout:
        return QStringLiteral("关于");
    case ApplicationAction::kCheckUpdates:
        return QStringLiteral("检查更新");
    case ApplicationAction::kRestart:
        return QStringLiteral("重新启动");
    case ApplicationAction::kUnlockPro:
        return QStringLiteral("升级 PRO");
    case ApplicationAction::kQuit:
        return QStringLiteral("退出");
    }
    return {};
}

QString actionObjectName(ApplicationAction action)
{
    switch (action) {
    case ApplicationAction::kSnip:
        return QStringLiteral("btn_AutoCapture");
    case ApplicationAction::kCaptureActiveWindow:
        return QStringLiteral("btn_ActiveWindow");
    case ApplicationAction::kScrollSnip:
        return QStringLiteral("btn_ScrollCapture");
    case ApplicationAction::kOcrSnip:
        return QStringLiteral("btn_Ocr");
    case ApplicationAction::kPickColor:
        return QStringLiteral("btn_ColorPicker");
    case ApplicationAction::kScreenRuler:
        return QStringLiteral("btn_ScreenRuler");
    case ApplicationAction::kPreferences:
        return QStringLiteral("btn_Preferences");
    case ApplicationAction::kToggleCaptureBar:
        return QStringLiteral("action_ToggleCaptureBar");
    case ApplicationAction::kAbout:
        return QStringLiteral("action_About");
    case ApplicationAction::kQuit:
        return QStringLiteral("action_Quit");
    default:
        return QStringLiteral("btn_Action");
    }
}

bool isImplementedAction(ApplicationAction action)
{
    return action == ApplicationAction::kSnip || action == ApplicationAction::kCaptureActiveWindow
           || action == ApplicationAction::kPickColor || action == ApplicationAction::kScreenRuler
           || action == ApplicationAction::kPreferences || action == ApplicationAction::kToggleCaptureBar
           || action == ApplicationAction::kAbout || action == ApplicationAction::kQuit;
}
