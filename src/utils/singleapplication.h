#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <memory>
#include <QApplication>
#include <QLocalServer>
#include <QSharedMemory>

class ScreenshotWindow;
class SystemTray;
class ShortcutManager;
class EditorWindow;
class WindowManager;

class SingleApplication : public QApplication
{
    Q_OBJECT

public:
    SingleApplication(int &argc, char **argv, const QString &appKey);
    ~SingleApplication() override;

    bool isRunning();
    bool sendMessage(const QString &message);
    void initialize();

public slots:
    void startScreenshot();
    void quit();

protected:
    void initializeFonts();

signals:
    void messageReceived(const QString &message);

private slots:
    void handleConnection();
    void receiveMessage();

private:
    bool initializeSharedMemory();
    void cleanupSharedMemory();

    QString          appKey_;
    QSharedMemory    sharedMemory_;
    QLocalServer    *localServer_;
    static const int timeout_ = 1000;

    std::unique_ptr<SystemTray>       systemTray_;
    std::unique_ptr<ShortcutManager>  shortcutManager_;
    std::unique_ptr<ScreenshotWindow> m_screenshotWindow;
    std::unique_ptr<EditorWindow>     m_editWindow;
    std::unique_ptr<WindowManager>    m_windowManager;
};

#endif // SINGLEAPPLICATION_H
