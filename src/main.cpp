#include "app/applicationcontroller.h"
#include "app/diagnostics.h"
#include "app/singleapplication.h"

#include <QMessageBox>

#include <cstdlib>

int main(int argc, char *argv[])
{
    SingleApplication app(argc, argv, QStringLiteral("QSnapCraft"));
    app.setOrganizationName(QStringLiteral("QSnapCraft"));
    app.setOrganizationDomain(QStringLiteral("qsnapcraft.local"));
    app.setApplicationName(QStringLiteral("QSnapCraft"));
    app.setApplicationVersion(QStringLiteral(QSNAPCRAFT_VERSION));

    QString diagnosticsError;
    if (!installDiagnostics({}, &diagnosticsError)) {
        QMessageBox::warning(nullptr,
                             QStringLiteral("QSnapCraft"),
                             QStringLiteral("诊断日志初始化失败，应用将继续运行。\n%1").arg(diagnosticsError));
    } else {
        QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, []() { shutdownDiagnostics(); });
    }

    if (app.status() == SingleApplication::InstanceStatus::kFailed) {
        QMessageBox::critical(nullptr,
                              QStringLiteral("QSnapCraft"),
                              QStringLiteral("无法初始化单实例通信。"),
                              QMessageBox::Ok);
        return EXIT_FAILURE;
    }
    if (app.isSecondaryInstance()) {
        const bool quit_requested = QCoreApplication::arguments().contains(QStringLiteral("--quit"));
        return app.notifyPrimaryInstance(quit_requested ? QStringLiteral("quit") : QStringLiteral("activate"))
                   ? EXIT_SUCCESS
                   : EXIT_FAILURE;
    }

    ApplicationController controller(&app);
    QObject::connect(&app, &SingleApplication::messageReceived, &controller, &ApplicationController::handleMessage);
    if (!controller.initialize()) {
        return EXIT_FAILURE;
    }
    return app.exec();
}
