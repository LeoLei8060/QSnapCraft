#pragma once

#include <QString>

[[nodiscard]] QString getDiagnosticsDirectory();
[[nodiscard]] bool    installDiagnostics(const QString &directory = {}, QString *error = nullptr);
void                  shutdownDiagnostics();
