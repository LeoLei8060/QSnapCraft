#pragma once

#include "capture/captureoptions.h"

class QSettings;

CaptureOptions loadCaptureOptions(QSettings &settings);
void           saveCaptureOptions(QSettings &settings, const CaptureOptions &options);
