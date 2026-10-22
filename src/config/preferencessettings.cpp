#include "preferencessettings.h"

#include "snipsettings.h"

#include <QSettings>

ApplicationPreferences loadApplicationPreferences(QSettings &settings)
{
    ApplicationPreferences preferences;

    settings.beginGroup(QStringLiteral("General"));
    preferences.general.autoStart = settings.value(QStringLiteral("AutoStart"), preferences.general.autoStart).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Control"));
    preferences.control.captureHotkey
        = settings.value(QStringLiteral("CaptureHotkey"), preferences.control.captureHotkey).toString();
    preferences.control.captureCopyHotkey
        = settings.value(QStringLiteral("CaptureCopyHotkey"), preferences.control.captureCopyHotkey).toString();
    preferences.control.showLastPinHotkey
        = settings.value(QStringLiteral("ShowLastPinHotkey"), preferences.control.showLastPinHotkey).toString();
    preferences.control.showPinsHotkey
        = settings.value(QStringLiteral("ShowPinsHotkey"), preferences.control.showPinsHotkey).toString();
    preferences.control.transparencyHotkey
        = settings.value(QStringLiteral("TransparencyHotkey"), preferences.control.transparencyHotkey).toString();
    preferences.control.closePinHotkey
        = settings.value(QStringLiteral("ClosePinHotkey"), preferences.control.closePinHotkey).toString();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Interface"));
    preferences.captureBar.enabled
        = settings.value(QStringLiteral("capture_bar_enabled"), preferences.captureBar.enabled).toBool();
    preferences.captureBar.dockEdge
        = settings.value(QStringLiteral("capture_bar_dock"), preferences.captureBar.dockEdge).toString();
    preferences.captureBar.screenName
        = settings.value(QStringLiteral("capture_bar_screen"), preferences.captureBar.screenName).toString();
    preferences.captureBar.positionRatio
        = qBound(0.0,
                 settings.value(QStringLiteral("capture_bar_position"), preferences.captureBar.positionRatio).toDouble(),
                 1.0);
    preferences.captureBar.autoHide
        = settings.value(QStringLiteral("capture_bar_auto_hide"), preferences.captureBar.autoHide).toBool();
    preferences.captureBar.snapDistancePx
        = qBound(0,
                 settings.value(QStringLiteral("capture_bar_snap_distance_px"), preferences.captureBar.snapDistancePx)
                     .toInt(),
                 48);
    preferences.captureBar.hotspotWidthPx
        = qBound(2,
                 settings.value(QStringLiteral("capture_bar_hotspot_width_px"), preferences.captureBar.hotspotWidthPx)
                     .toInt(),
                 12);
    preferences.captureBar.hotspotLengthPx
        = qBound(20,
                 settings.value(QStringLiteral("capture_bar_hotspot_length_px"), preferences.captureBar.hotspotLengthPx)
                     .toInt(),
                 320);
    preferences.captureBar.hideDelayMs
        = qBound(100,
                 settings.value(QStringLiteral("capture_bar_hide_delay_ms"), preferences.captureBar.hideDelayMs).toInt(),
                 3000);
    preferences.captureBar.revealDelayMs
        = qBound(0,
                 settings.value(QStringLiteral("capture_bar_reveal_delay_ms"), preferences.captureBar.revealDelayMs)
                     .toInt(),
                 1000);
    preferences.captureBar.slideDurationMs
        = qBound(0,
                 settings.value(QStringLiteral("capture_bar_slide_duration_ms"), preferences.captureBar.slideDurationMs)
                     .toInt(),
                 1000);
    preferences.captureBar.freePosition
        = settings.value(QStringLiteral("capture_bar_free_position"), preferences.captureBar.freePosition).toPoint();
    preferences.captureBar.hasFreePosition
        = settings.value(QStringLiteral("capture_bar_has_free_position"), preferences.captureBar.hasFreePosition)
              .toBool();
    preferences.captureBar.horizontal
        = settings.value(QStringLiteral("capture_bar_horizontal"), preferences.captureBar.horizontal).toBool();
    settings.endGroup();

    preferences.capture = loadCaptureOptions(settings);
    return preferences;
}

void saveApplicationPreferences(QSettings &settings, const ApplicationPreferences &preferences)
{
    settings.beginGroup(QStringLiteral("General"));
    settings.setValue(QStringLiteral("AutoStart"), preferences.general.autoStart);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Control"));
    settings.setValue(QStringLiteral("CaptureHotkey"), preferences.control.captureHotkey);
    settings.setValue(QStringLiteral("CaptureCopyHotkey"), preferences.control.captureCopyHotkey);
    settings.setValue(QStringLiteral("ShowLastPinHotkey"), preferences.control.showLastPinHotkey);
    settings.setValue(QStringLiteral("ShowPinsHotkey"), preferences.control.showPinsHotkey);
    settings.setValue(QStringLiteral("TransparencyHotkey"), preferences.control.transparencyHotkey);
    settings.setValue(QStringLiteral("ClosePinHotkey"), preferences.control.closePinHotkey);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Interface"));
    settings.setValue(QStringLiteral("capture_bar_enabled"), preferences.captureBar.enabled);
    settings.setValue(QStringLiteral("capture_bar_dock"), preferences.captureBar.dockEdge);
    settings.setValue(QStringLiteral("capture_bar_screen"), preferences.captureBar.screenName);
    settings.setValue(QStringLiteral("capture_bar_position"), preferences.captureBar.positionRatio);
    settings.setValue(QStringLiteral("capture_bar_auto_hide"), preferences.captureBar.autoHide);
    settings.setValue(QStringLiteral("capture_bar_snap_distance_px"), preferences.captureBar.snapDistancePx);
    settings.setValue(QStringLiteral("capture_bar_hotspot_width_px"), preferences.captureBar.hotspotWidthPx);
    settings.setValue(QStringLiteral("capture_bar_hotspot_length_px"), preferences.captureBar.hotspotLengthPx);
    settings.setValue(QStringLiteral("capture_bar_hide_delay_ms"), preferences.captureBar.hideDelayMs);
    settings.setValue(QStringLiteral("capture_bar_reveal_delay_ms"), preferences.captureBar.revealDelayMs);
    settings.setValue(QStringLiteral("capture_bar_slide_duration_ms"), preferences.captureBar.slideDurationMs);
    settings.setValue(QStringLiteral("capture_bar_free_position"), preferences.captureBar.freePosition);
    settings.setValue(QStringLiteral("capture_bar_has_free_position"), preferences.captureBar.hasFreePosition);
    settings.setValue(QStringLiteral("capture_bar_horizontal"), preferences.captureBar.horizontal);
    settings.endGroup();

    saveCaptureOptions(settings, preferences.capture);
}
