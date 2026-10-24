#include "systemtray.h"

#include "ui/glyphicon.h"

#include <QAction>
#include <QSignalBlocker>
#include <QVariant>

SystemTray::SystemTray(QObject *parent)
    : QObject(parent)
{
    createTrayIcon();
}

void SystemTray::show()
{
    m_trayIcon->show();
}

void SystemTray::closeContextMenu()
{
    if (m_trayMenu) {
        m_trayMenu->close();
    }
}

void SystemTray::setCaptureBarVisible(bool visible)
{
    if (m_toggleCaptureBarAction) {
        const QSignalBlocker blocker(m_toggleCaptureBarAction);
        m_toggleCaptureBarAction->setChecked(visible);
    }
}

void SystemTray::setHotkeysEnabled(bool enabled)
{
    if (m_toggleHotkeysAction) {
        const QSignalBlocker blocker(m_toggleHotkeysAction);
        m_toggleHotkeysAction->setChecked(enabled);
    }
}

void SystemTray::setOcrAvailable(bool available)
{
    if (m_ocrAction == nullptr) {
        return;
    }
    m_ocrAction->setEnabled(available);
    m_ocrAction->setToolTip(available ? actionDisplayName(ApplicationAction::kOcrSnip)
                                      : QStringLiteral("OCR（未安装 Windows OCR 语言包）"));
}

void SystemTray::showMessage(const QString &title, const QString &message)
{
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Warning, 5000);
}

QMenu *SystemTray::contextMenu() const
{
    return m_trayMenu.get();
}

QAction *SystemTray::addAction(ApplicationAction action, bool checkable)
{
    auto *item = new QAction(actionDisplayName(action), this);
    item->setObjectName(actionObjectName(action));
    item->setData(QVariant::fromValue(action));
    item->setIcon(createGlyphIcon(applicationActionGlyph(action)));
    item->setCheckable(checkable);
    item->setEnabled(isImplementedAction(action));
    item->setToolTip(isImplementedAction(action) ? actionDisplayName(action)
                                                 : QStringLiteral("%1（功能开发中）").arg(actionDisplayName(action)));
    connect(item, &QAction::triggered, this, &SystemTray::onActionTriggered);
    m_trayMenu->addAction(item);
    return item;
}

void SystemTray::createTrayIcon()
{
    m_trayMenu = std::make_unique<QMenu>();
    m_trayMenu->setObjectName(QStringLiteral("menu_Tray"));
    m_trayMenu->setToolTipsVisible(true);
    m_trayMenu->setStyleSheet(QStringLiteral("QMenu { background: #292929; color: #E8E8E8; border: 1px solid #555;"
                                             " padding: 5px; }"
                                             "QMenu::item { padding: 6px 28px 6px 10px; border-radius: 4px; }"
                                             "QMenu::item:selected { background: #0A84FF; color: white; }"
                                             "QMenu::item:disabled { color: #777777; }"
                                             "QMenu::separator { height: 1px; background: #444; margin: 4px 8px; }"));

    addAction(ApplicationAction::kPaste);
    addAction(ApplicationAction::kSnip);
    addAction(ApplicationAction::kCaptureActiveWindow);
    addAction(ApplicationAction::kScrollSnip);
    m_ocrAction = addAction(ApplicationAction::kOcrSnip);
    m_trayMenu->addSeparator();
    addAction(ApplicationAction::kHideAllImages);
    addAction(ApplicationAction::kShowAllImages);
    addAction(ApplicationAction::kRecoverImage);
    m_trayMenu->addSeparator();
    addAction(ApplicationAction::kPickColor);
    addAction(ApplicationAction::kScreenRuler);
    m_trayMenu->addSeparator();
    m_toggleCaptureBarAction = addAction(ApplicationAction::kToggleCaptureBar, true);
    m_toggleHotkeysAction = addAction(ApplicationAction::kToggleHotkeys, true);
    addAction(ApplicationAction::kPreferences);
    m_trayMenu->addSeparator();
    addAction(ApplicationAction::kAbout);
    m_trayMenu->addSeparator();
    addAction(ApplicationAction::kQuit);

    m_trayIcon = std::make_unique<QSystemTrayIcon>();
    m_trayIcon->setContextMenu(m_trayMenu.get());
    m_trayIcon->setIcon(QIcon(":/icons/QSnapCraft.ico"));
    m_trayIcon->setToolTip("QSnapCraft");

    connect(m_trayIcon.get(), &QSystemTrayIcon::activated, this, &SystemTray::onTrayIconActivated);
}

void SystemTray::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        emit sigActionTriggered(ApplicationAction::kSnip);
    } else if (reason == QSystemTrayIcon::MiddleClick) {
        emit sigActionTriggered(ApplicationAction::kPaste);
    }
}

void SystemTray::onActionTriggered()
{
    const auto *action = qobject_cast<const QAction *>(sender());
    if (!action) {
        return;
    }
    emit sigActionTriggered(action->data().value<ApplicationAction>());
}
