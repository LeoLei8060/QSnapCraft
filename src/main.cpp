#include "utils/SingleApplication.h"
#include "utils/FontManager.h"
#include <QMessageBox>
#include <QDebug>

int main(int argc, char *argv[])
{
    SingleApplication app(argc, argv, "QSnapCraft");

    // 加载字体文件
    if (!FontManager::instance()->addThirdpartyFont(":/iconfont/iconfont.ttf", FontManager::IconFont)) {
        qWarning() << "Failed to load icon font";
    }

    if (app.isRunning()) {
        QMessageBox::warning(nullptr,
                           "QSnapCraft",
                           "Application is already running.",
                           QMessageBox::Ok);
        return -1;
    }

    return app.exec();
}
