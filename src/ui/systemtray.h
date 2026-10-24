#pragma once

#include "app/applicationaction.h"

#include <memory>
#include <QMenu>
#include <QSystemTrayIcon>

class QAction;

class SystemTray : public QObject
{
    Q_OBJECT

public:
    explicit SystemTray(QObject *parent = nullptr);
    ~SystemTray() override = default;

    void   show();
    void   closeContextMenu();
    void   setCaptureBarVisible(bool visible);
    void   setHotkeysEnabled(bool enabled);
    void   setOcrAvailable(bool available);
    void   showMessage(const QString &title, const QString &message);
    QMenu *contextMenu() const;

signals:
    void sigActionTriggered(ApplicationAction action);

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onActionTriggered();

private:
    QAction *addAction(ApplicationAction action, bool checkable = false);
    void     createTrayIcon();

    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
    std::unique_ptr<QMenu>           m_trayMenu;
    QAction                         *m_toggleCaptureBarAction{nullptr};
    QAction                         *m_toggleHotkeysAction{nullptr};
    QAction                         *m_ocrAction{nullptr};
};
