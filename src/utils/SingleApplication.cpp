#include "SingleApplication.h"
#include "core/screenshot/screenshot_window.h"
#include "shortcut_manager.h"
#include "system_tray.h"
#include <QDataStream>
#include <QLocalSocket>

SingleApplication::SingleApplication(int &argc, char **argv, const QString &appKey)
    : QApplication(argc, argv)
    , appKey_(appKey)
    , sharedMemory_(appKey)
    , localServer_(nullptr)
    , systemTray_(nullptr)
    , shortcutManager_(nullptr)
    , screenshotWindow_(nullptr)
{
    initializeSharedMemory();
    initialize();
}

SingleApplication::~SingleApplication()
{
    cleanupSharedMemory();
    if (localServer_) {
        localServer_->close();
        delete localServer_;
    }
}

bool SingleApplication::initializeSharedMemory()
{
    if (sharedMemory_.attach()) {
        return false;
    }

    if (!sharedMemory_.create(1)) {
        return false;
    }

    localServer_ = new QLocalServer(this);
    connect(localServer_, &QLocalServer::newConnection, this, &SingleApplication::handleConnection);

    QLocalServer::removeServer(appKey_);

    if (!localServer_->listen(appKey_)) {
        return false;
    }

    return true;
}

void SingleApplication::cleanupSharedMemory()
{
    if (sharedMemory_.isAttached()) {
        sharedMemory_.detach();
    }
}

bool SingleApplication::isRunning()
{
    return !localServer_;
}

bool SingleApplication::sendMessage(const QString &message)
{
    if (!isRunning()) {
        return false;
    }

    QLocalSocket socket;
    socket.connectToServer(appKey_, QIODevice::WriteOnly);

    if (!socket.waitForConnected(timeout_)) {
        return false;
    }

    QByteArray  buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream << message;

    socket.write(buffer);
    if (!socket.waitForBytesWritten(timeout_)) {
        return false;
    }

    socket.disconnectFromServer();
    return true;
}

void SingleApplication::handleConnection()
{
    QLocalSocket *socket = localServer_->nextPendingConnection();
    if (!socket) {
        return;
    }

    connect(socket, &QLocalSocket::readyRead, this, &SingleApplication::receiveMessage);
    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

void SingleApplication::receiveMessage()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket) {
        return;
    }

    QByteArray  buffer = socket->readAll();
    QDataStream stream(buffer);
    QString     message;
    stream >> message;

    emit messageReceived(message);
}

void SingleApplication::initialize()
{
    systemTray_ = std::make_unique<SystemTray>();
    systemTray_->show();

    shortcutManager_ = std::make_unique<ShortcutManager>();

    connect(shortcutManager_.get(),
            &ShortcutManager::screenshotTriggered,
            this,
            &SingleApplication::startScreenshot);
}

void SingleApplication::startScreenshot()
{
    if (!screenshotWindow_) {
        screenshotWindow_ = std::make_unique<ScreenshotWindow>();
    }
    screenshotWindow_->start();
}
