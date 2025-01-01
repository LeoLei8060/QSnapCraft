#include "utils/singleapplication.h"
#include <QDebug>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    SingleApplication app(argc, argv, "QSnapCraft");

    if (app.isRunning()) {
        QMessageBox::warning(nullptr,
                             "QSnapCraft",
                             "Application is already running.",
                             QMessageBox::Ok);
        return -1;
    }

    return app.exec();
}
