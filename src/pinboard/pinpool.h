#pragma once

#include "config/pinoptions.h"

#include <QLineF>
#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QRect>
#include <QTimer>
#include <QVector>

#include <memory>
#include <vector>

class PinWindow;
class PinSnapOverlay;

class PinPool : public QObject
{
    Q_OBJECT

public:
    explicit PinPool(const QString &storageDirectory = {}, QObject *parent = nullptr);
    ~PinPool() override;

    PinWindow                   *pinImage(const QPixmap &pixmap, const QPoint &position, const QSize &logicalSize = {});
    int                          pinCount() const;
    void                         setRecoveryLimit(int limit);
    const QVector<PinGroupData> &groups() const;
    QString                      createGroup(const QString &name);
    bool                         renameGroup(const QString &groupId, const QString &name);
    void                         removeGroup(const QString &groupId);
    void                         setGroupVisible(const QString &groupId, bool visible);
    void                         closeGroup(const QString &groupId);

public slots:
    bool pasteFromClipboard();
    void hideAllPins();
    void showAllPins();
    void togglePinsVisibility();
    void assignPinGroup(PinWindow *window, const QString &groupId);
    bool recoverLastPin();
    bool closePinAt(const QPoint &globalPosition);
    bool toggleClickThroughAt(const QPoint &global_position);
    void ensurePinsVisible();
    void applyPinSettings();

private:
    void       removeClosedPin(PinWindow *window);
    void       removeNullPins();
    void       pruneRecoveryPool();
    void       refreshSnapTargets();
    void       updateSnapOverlay(PinWindow *window, const QVector<QLineF> &guides);
    PinWindow *pinWindowAt(const QPoint &globalPosition);
    void       schedulePersistenceSave();
    void       savePersistedPins();
    void       loadPersistedPins();
    void       createGroupForPin(PinWindow *window);
    void       refreshGroupMenus();
    void       handleGroupAction(PinWindow *window, const QString &groupId, PinGroupAction action);
    void       refreshForeignSnapTargets();

    std::vector<QPointer<PinWindow>> m_pinWindows;
    std::vector<QPointer<PinWindow>> m_recentClosedPins;
    std::vector<QPointer<PinWindow>> m_bulkHiddenPins;
    std::unique_ptr<PinSnapOverlay>  m_snapOverlay;
    int                              m_recoveryLimit{20};
    bool                             m_lastShowShadow{true};
    QString                          m_storageDirectory;
    QVector<PinGroupData>            m_groups;
    QTimer                           m_saveTimer;
    QTimer                           m_foreignSnapTimer;
    QVector<QRect>                   m_foreignSnapTargets;
    bool                             m_loadingPersistence{false};
};
