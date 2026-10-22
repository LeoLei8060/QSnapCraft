#include "singleapplication.h"

#include <QDataStream>
#include <QLocalSocket>

SingleApplication::SingleApplication(int &argc, char **argv, const QString &appKey)
    : QApplication(argc, argv)
    , m_appKey(appKey)
    , m_sharedMemory(appKey)
{
    setQuitOnLastWindowClosed(false);
    m_status = initializeSingleInstance();
}

SingleApplication::~SingleApplication()
{
    if (m_localServer != nullptr) {
        m_localServer->close();
    }
    if (m_sharedMemory.isAttached()) {
        m_sharedMemory.detach();
    }
}

SingleApplication::InstanceStatus SingleApplication::status() const
{
    return m_status;
}

bool SingleApplication::isPrimaryInstance() const
{
    return m_status == InstanceStatus::kPrimary;
}

bool SingleApplication::isSecondaryInstance() const
{
    return m_status == InstanceStatus::kSecondary;
}

bool SingleApplication::notifyPrimaryInstance(const QString &message)
{
    return sendMessage(message);
}

bool SingleApplication::sendMessage(const QString &message)
{
    if (!isSecondaryInstance()) {
        return false;
    }

    QLocalSocket socket;
    socket.connectToServer(m_appKey, QIODevice::WriteOnly);
    if (!socket.waitForConnected(TIMEOUT_MS)) {
        return false;
    }

    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream << message;
    socket.write(buffer);
    if (!socket.waitForBytesWritten(TIMEOUT_MS)) {
        return false;
    }

    socket.disconnectFromServer();
    return true;
}

void SingleApplication::handleConnection()
{
    QLocalSocket *socket = m_localServer->nextPendingConnection();
    if (socket == nullptr) {
        return;
    }

    connect(socket, &QLocalSocket::readyRead, this, &SingleApplication::receiveMessage);
    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

void SingleApplication::receiveMessage()
{
    auto *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket == nullptr) {
        return;
    }

    const QByteArray buffer = socket->readAll();
    QDataStream stream(buffer);
    QString message;
    stream >> message;
    emit messageReceived(message);
}

SingleApplication::InstanceStatus SingleApplication::initializeSingleInstance()
{
    if (m_sharedMemory.attach()) {
        return InstanceStatus::kSecondary;
    }

    if (!m_sharedMemory.create(1)) {
        return m_sharedMemory.error() == QSharedMemory::AlreadyExists
                   ? InstanceStatus::kSecondary
                   : InstanceStatus::kFailed;
    }

    m_localServer = new QLocalServer(this);
    connect(m_localServer, &QLocalServer::newConnection, this, &SingleApplication::handleConnection);
    QLocalServer::removeServer(m_appKey);
    if (!m_localServer->listen(m_appKey)) {
        delete m_localServer;
        m_localServer = nullptr;
        if (m_sharedMemory.isAttached()) {
            m_sharedMemory.detach();
        }
        return InstanceStatus::kFailed;
    }
    return InstanceStatus::kPrimary;
}
