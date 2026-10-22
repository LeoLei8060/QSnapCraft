#pragma once

#include <QApplication>
#include <QLocalServer>
#include <QSharedMemory>
#include <QString>

class QLocalSocket;

class SingleApplication : public QApplication
{
    Q_OBJECT

public:
    enum class InstanceStatus {
        kPrimary,
        kSecondary,
        kFailed
    };

    SingleApplication(int &argc, char **argv, const QString &appKey);
    ~SingleApplication() override;

    InstanceStatus status() const;
    bool           isPrimaryInstance() const;
    bool           isSecondaryInstance() const;
    bool           notifyPrimaryInstance(const QString &message = QStringLiteral("activate"));
    bool sendMessage(const QString &message);

signals:
    void messageReceived(const QString &message);

private slots:
    void handleConnection();
    void receiveMessage();

private:
    InstanceStatus initializeSingleInstance();

    static constexpr int TIMEOUT_MS = 1000;

    QString       m_appKey;
    QSharedMemory m_sharedMemory;
    QLocalServer *m_localServer{nullptr};
    InstanceStatus m_status{InstanceStatus::kFailed};
};
