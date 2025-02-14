#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <memory>
#include <QApplication>
#include <QLocalServer>
#include <QSharedMemory>

class ScreenshotWindow;
class SystemTray;
class EditorWindow;
class WindowManager;
class GlobalConfig;

class SingleApplication : public QApplication
{
    Q_OBJECT

public:
    SingleApplication(int &argc, char **argv, const QString &appKey);
    ~SingleApplication() override;

    bool isRunning();
    bool sendMessage(const QString &message);

    // 初始化和清理
    void initialize();
    void cleanup();

public slots:
    void startScreenshot();
    void quit();

protected:
    void initializeFonts();
    void initializeConfig(); // 初始化配置
    void initializeHotKey();

signals:
    void messageReceived(const QString &message);

private slots:
    void handleConnection();
    void receiveMessage();

private:
    bool initializeSharedMemory();
    void cleanupSharedMemory();

    QString          m_appKey;
    QSharedMemory    m_sharedMemory;
    QLocalServer    *m_localServer;
    static const int m_timeout = 1000;

    // 全局组件
    std::unique_ptr<GlobalConfig>     globalConfig_; // 全局配置
    std::unique_ptr<SystemTray>       systemTray_;   // 系统托盘
    std::unique_ptr<ScreenshotWindow> m_screenshotWindow;
    std::unique_ptr<EditorWindow>     m_editWindow;
    std::unique_ptr<WindowManager>    m_windowManager;
};

#endif // SINGLEAPPLICATION_H
