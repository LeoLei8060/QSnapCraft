#include "app/diagnostics.h"

#include <QCoreApplication>

#include <windows.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QSnapCraft"));
    app.setApplicationVersion(QStringLiteral(QSNAPCRAFT_VERSION));

    const QString directory = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    QString       error;
    if (!installDiagnostics(directory, &error)) {
        return 2;
    }

    RaiseException(0xE0000001, EXCEPTION_NONCONTINUABLE, 0, nullptr);
    return 3;
}
