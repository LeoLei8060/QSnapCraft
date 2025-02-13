#include "SingleApplication.h"
#include "core/editor/editorwindow.h"
#include "core/screenshot/screenshotwindow.h"
#include "core/windowmanager.h"
#include "settings/globalconfig.h"
#include "shortcutmanager.h"
#include "systemtray.h"
#include "utils/fontmanager.h"
#include "utils/systemutils.h"
#include <QDataStream>
#include <QDebug>
#include <QLocalSocket>

SingleApplication::SingleApplication(int &argc, char **argv, const QString &appKey)
    : QApplication(argc, argv)
    , m_appKey(appKey)
    , m_sharedMemory(appKey)
    , m_localServer(nullptr)
    , globalConfig_(nullptr)
    , systemTray_(nullptr)
    , shortcutManager_(nullptr)
    , m_screenshotWindow(nullptr)
{
    initializeSharedMemory();
    initialize();
}

SingleApplication::~SingleApplication()
{
    cleanup();
}

void SingleApplication::cleanup()
{
    // 清理全局配置
    globalConfig_.reset();

    // 清理其他资源
    cleanupSharedMemory();
    if (m_localServer) {
        m_localServer->close();
        delete m_localServer;
    }
}

bool SingleApplication::initializeSharedMemory()
{
    if (m_sharedMemory.attach()) {
        return false;
    }

    if (!m_sharedMemory.create(1)) {
        return false;
    }

    m_localServer = new QLocalServer(this);
    connect(m_localServer, &QLocalServer::newConnection, this, &SingleApplication::handleConnection);

    QLocalServer::removeServer(m_appKey);

    if (!m_localServer->listen(m_appKey)) {
        return false;
    }

    return true;
}

void SingleApplication::cleanupSharedMemory()
{
    if (m_sharedMemory.isAttached()) {
        m_sharedMemory.detach();
    }
}

bool SingleApplication::isRunning()
{
    return !m_localServer;
}

bool SingleApplication::sendMessage(const QString &message)
{
    if (!isRunning()) {
        return false;
    }

    QLocalSocket socket;
    socket.connectToServer(m_appKey, QIODevice::WriteOnly);

    if (!socket.waitForConnected(m_timeout)) {
        return false;
    }

    QByteArray  buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    stream << message;

    socket.write(buffer);
    if (!socket.waitForBytesWritten(m_timeout)) {
        return false;
    }

    socket.disconnectFromServer();
    return true;
}

void SingleApplication::handleConnection()
{
    QLocalSocket *socket = m_localServer->nextPendingConnection();
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
    if (isRunning())
        return;

    // 初始化全局配置（最先初始化，因为其他组件可能需要用到配置）
    initializeConfig();

    // 同步开机自启动状态
    const auto &generalData = GlobalConfig::instance()->generalData();
    SystemUtils::setAutoStart(generalData.autoStart);

    // 初始化字体
    initializeFonts();

    // 初始化系统托盘
    systemTray_ = std::make_unique<SystemTray>();

    // 初始化快捷键管理器
    shortcutManager_ = std::make_unique<ShortcutManager>();

    // 初始化窗口管理器
    m_windowManager = std::make_unique<WindowManager>();

    // 连接信号槽
    connect(shortcutManager_.get(),
            &ShortcutManager::screenshotTriggered,
            this,
            &SingleApplication::startScreenshot);

    connect(shortcutManager_.get(),
            &ShortcutManager::escapePressed,
            m_windowManager.get(),
            &WindowManager::onEscapePressed);

    connect(systemTray_.get(),
            &SystemTray::sigSettingActTriggered,
            m_windowManager.get(),
            &WindowManager::onSettingActTriggered);

    systemTray_->show();
}

void SingleApplication::initializeConfig()
{
    // 创建全局配置实例
    globalConfig_ = std::unique_ptr<GlobalConfig>(GlobalConfig::instance());
}

void SingleApplication::quit() {}

void SingleApplication::initializeFonts()
{
    // 加载字体文件
    if (!FontManager::instance()->addThirdpartyFont(":/iconfont/iconfont.ttf",
                                                    FontManager::IconFont)) {
        qWarning() << "Failed to load icon font";
    }
}

void SingleApplication::startScreenshot()
{
    m_windowManager->startCapture();
}
