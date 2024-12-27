#include "utils/SingleApplication.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
    SingleApplication app(argc, argv, "QSnapCraft");

    if (app.isRunning()) {
        QMessageBox::warning(nullptr,
                             "QSnapCraft",
                             "Another instance of QSnapCraft is already running.");
        return 1;
    }

    return app.exec();
}
