#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

enum class ApplicationAction {
    kSnip,
    kCaptureActiveWindow,
    kScrollSnip,
    kOcrSnip,
    kPaste,
    kPickColor,
    kScreenRuler,
    kToggleCaptureBar,
    kHideAllImages,
    kShowAllImages,
    kRecoverImage,
    kPreferences,
    kToggleHotkeys,
    kHelp,
    kAbout,
    kCheckUpdates,
    kRestart,
    kUnlockPro,
    kQuit
};

Q_DECLARE_METATYPE(ApplicationAction)

QVector<ApplicationAction> captureBarActions();
QString                    actionDisplayName(ApplicationAction action);
QString                    actionObjectName(ApplicationAction action);
bool                       isImplementedAction(ApplicationAction action);
