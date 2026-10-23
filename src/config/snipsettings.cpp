#include "snipsettings.h"

#include <QColor>
#include <QSettings>

CaptureOptions loadCaptureOptions(QSettings &settings)
{
    CaptureOptions options;
    const auto     outputAction = [](int value) {
        return static_cast<CaptureOutputAction>(qBound(0, value, static_cast<int>(CaptureOutputAction::kQuickSave)));
    };

    settings.beginGroup(QStringLiteral("Snip"));
    options.detectWindows = settings.value(QStringLiteral("detect_windows"), options.detectWindows).toBool();
    options.detectUiElements = settings.value(QStringLiteral("detect_ui_elements"), options.detectUiElements).toBool();
    options.showAnchors = settings.value(QStringLiteral("show_anchors"), options.showAnchors).toBool();
    options.showTips = settings.value(QStringLiteral("show_tips"), options.showTips).toBool();
    options.hide1x1 = settings.value(QStringLiteral("hide_1x1"), options.hide1x1).toBool();
    options.drawOutsideArea = settings.value(QStringLiteral("draw_outside_area"), options.drawOutsideArea).toBool();
    options.autoCrossLines = settings.value(QStringLiteral("auto_cross_lines"), options.autoCrossLines).toBool();
    options.askForConfirmOnEscape
        = settings.value(QStringLiteral("ask_for_confirm_on_esc"), options.askForConfirmOnEscape).toBool();
    options.abortOnFocusOut = settings.value(QStringLiteral("abort_on_focus_out"), options.abortOnFocusOut).toBool();
    options.loopHistoryAreas = settings.value(QStringLiteral("loop_history_areas"), options.loopHistoryAreas).toBool();
    options.enterKeyOutput = settings.value(QStringLiteral("enter_key_output"), options.enterKeyOutput).toBool();
    options.doubleClickOutput = settings.value(QStringLiteral("double_click_output"), options.doubleClickOutput).toBool();
    options.middleClickOutput = settings.value(QStringLiteral("middle_click_output"), options.middleClickOutput).toBool();
    options.enterAction = outputAction(
        settings.value(QStringLiteral("enter_action"), static_cast<int>(options.enterAction)).toInt());
    options.doubleClickAction = outputAction(
        settings.value(QStringLiteral("double_click_action"), static_cast<int>(options.doubleClickAction)).toInt());
    options.middleClickAction = outputAction(
        settings.value(QStringLiteral("middle_click_action"), static_cast<int>(options.middleClickAction)).toInt());
    options.enterExit = settings.value(QStringLiteral("enter_exit"), options.enterExit).toBool();
    options.doubleClickExit = settings.value(QStringLiteral("double_click_exit"), options.doubleClickExit).toBool();
    options.middleClickExit = settings.value(QStringLiteral("middle_click_exit"), options.middleClickExit).toBool();
    options.captureCursor = settings.value(QStringLiteral("capture_cursor"), options.captureCursor).toBool();
    options.neverCaptureCursor = settings.value(QStringLiteral("never_capture_cursor"), options.neverCaptureCursor)
                                     .toBool();
    options.showMagnifier = settings.value(QStringLiteral("show_magnifier"), options.showMagnifier).toBool();
    options.showMaskInMagnifier = settings.value(QStringLiteral("show_mask_in_magnifier"), options.showMaskInMagnifier)
                                      .toBool();
    options.showBorderInMagnifier
        = settings.value(QStringLiteral("show_border_in_magnifier"), options.showBorderInMagnifier).toBool();
    options.showAnchorsInMagnifier
        = settings.value(QStringLiteral("show_anchors_in_magnifier"), options.showAnchorsInMagnifier).toBool();
    options.disableMouseScroll = settings.value(QStringLiteral("disable_mouse_scroll"), options.disableMouseScroll)
                                     .toBool();
    options.autoSave = settings.value(QStringLiteral("auto_save"), options.autoSave).toBool();
    options.soundEnabled = settings.value(QStringLiteral("play_sound"), options.soundEnabled).toBool();
    options.maxHistoryAreas = qMax(1, settings.value(QStringLiteral("max_areas"), options.maxHistoryAreas).toInt());
    options.borderWidth = qMax(1, settings.value(QStringLiteral("border_width"), options.borderWidth).toInt());
    options.anchorBorderColor = QColor(
        settings.value(QStringLiteral("anchor_border_color"), options.anchorBorderColor.name(QColor::HexArgb))
            .toString());
    if (!options.anchorBorderColor.isValid()) {
        options.anchorBorderColor = QColor(0, 0, 0, 255);
    }
    options.quickSavePath = settings.value(QStringLiteral("quick_save_path"), options.quickSavePath).toString();
    options.manualSavePath = settings.value(QStringLiteral("manual_save_path"), options.manualSavePath).toString();
    options.autoSavePath = settings.value(QStringLiteral("auto_save_path"), options.autoSavePath).toString();
    options.filenamePattern = settings.value(QStringLiteral("filename_pattern"), options.filenamePattern).toString();
    options.imageFormat = settings.value(QStringLiteral("format"), options.imageFormat).toString();
    options.jpegQuality = qBound(1, settings.value(QStringLiteral("jpeg_quality"), options.jpegQuality).toInt(), 100);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Misc"));
    options.uppercaseHexColor = settings.value(QStringLiteral("uppercase_hex_color"), options.uppercaseHexColor).toBool();
    options.physicalSelectionUnits
        = settings.value(QStringLiteral("use_physical_selection_size"), options.physicalSelectionUnits).toBool();
    settings.endGroup();
    return options;
}

void saveCaptureOptions(QSettings &settings, const CaptureOptions &options)
{
    settings.beginGroup(QStringLiteral("Snip"));
    settings.setValue(QStringLiteral("detect_windows"), options.detectWindows);
    settings.setValue(QStringLiteral("detect_ui_elements"), options.detectUiElements);
    settings.setValue(QStringLiteral("show_anchors"), options.showAnchors);
    settings.setValue(QStringLiteral("show_tips"), options.showTips);
    settings.setValue(QStringLiteral("hide_1x1"), options.hide1x1);
    settings.setValue(QStringLiteral("draw_outside_area"), options.drawOutsideArea);
    settings.setValue(QStringLiteral("auto_cross_lines"), options.autoCrossLines);
    settings.setValue(QStringLiteral("ask_for_confirm_on_esc"), options.askForConfirmOnEscape);
    settings.setValue(QStringLiteral("abort_on_focus_out"), options.abortOnFocusOut);
    settings.setValue(QStringLiteral("loop_history_areas"), options.loopHistoryAreas);
    settings.setValue(QStringLiteral("enter_key_output"), options.enterKeyOutput);
    settings.setValue(QStringLiteral("double_click_output"), options.doubleClickOutput);
    settings.setValue(QStringLiteral("middle_click_output"), options.middleClickOutput);
    settings.setValue(QStringLiteral("enter_action"), static_cast<int>(options.enterAction));
    settings.setValue(QStringLiteral("double_click_action"), static_cast<int>(options.doubleClickAction));
    settings.setValue(QStringLiteral("middle_click_action"), static_cast<int>(options.middleClickAction));
    settings.setValue(QStringLiteral("enter_exit"), options.enterExit);
    settings.setValue(QStringLiteral("double_click_exit"), options.doubleClickExit);
    settings.setValue(QStringLiteral("middle_click_exit"), options.middleClickExit);
    settings.setValue(QStringLiteral("capture_cursor"), options.captureCursor);
    settings.setValue(QStringLiteral("never_capture_cursor"), options.neverCaptureCursor);
    settings.setValue(QStringLiteral("show_magnifier"), options.showMagnifier);
    settings.setValue(QStringLiteral("show_mask_in_magnifier"), options.showMaskInMagnifier);
    settings.setValue(QStringLiteral("show_border_in_magnifier"), options.showBorderInMagnifier);
    settings.setValue(QStringLiteral("show_anchors_in_magnifier"), options.showAnchorsInMagnifier);
    settings.setValue(QStringLiteral("disable_mouse_scroll"), options.disableMouseScroll);
    settings.setValue(QStringLiteral("auto_save"), options.autoSave);
    settings.setValue(QStringLiteral("play_sound"), options.soundEnabled);
    settings.setValue(QStringLiteral("max_areas"), qMax(1, options.maxHistoryAreas));
    settings.setValue(QStringLiteral("border_width"), qMax(1, options.borderWidth));
    settings.setValue(QStringLiteral("anchor_border_color"), options.anchorBorderColor.name(QColor::HexArgb));
    settings.setValue(QStringLiteral("quick_save_path"), options.quickSavePath);
    settings.setValue(QStringLiteral("manual_save_path"), options.manualSavePath);
    settings.setValue(QStringLiteral("auto_save_path"), options.autoSavePath);
    settings.setValue(QStringLiteral("filename_pattern"), options.filenamePattern);
    settings.setValue(QStringLiteral("format"), options.imageFormat);
    settings.setValue(QStringLiteral("jpeg_quality"), qBound(1, options.jpegQuality, 100));
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Misc"));
    settings.setValue(QStringLiteral("uppercase_hex_color"), options.uppercaseHexColor);
    settings.setValue(QStringLiteral("use_physical_selection_size"), options.physicalSelectionUnits);
    settings.endGroup();
}
