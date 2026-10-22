#pragma once

#include "config/settingsdata.h"

class QSettings;

ApplicationPreferences loadApplicationPreferences(QSettings &settings);
void                   saveApplicationPreferences(QSettings &settings, const ApplicationPreferences &preferences);
