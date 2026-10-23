#pragma once

#include <QLineF>
#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QRect>
#include <QVector>

#include <memory>
#include <vector>

class PinWindow;
class PinSnapOverlay;

class PinPool : public QObject
{
    Q_OBJECT

public:
    explicit PinPool(QObject *parent = nullptr);
    ~PinPool() override;

    void pinImage(const QPixmap &pixmap, const QPoint &position, const QSize &logicalSize = {});
    int  pinCount() const;
    void setRecoveryLimit(int limit);

public slots:
    bool pasteFromClipboard();
    void hideAllPins();
    void showAllPins();
    void togglePinsVisibility();
    bool recoverLastPin();
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
    QRect      virtualDesktopGeometry() const;

    std::vector<QPointer<PinWindow>> m_pinWindows;
    std::vector<QPointer<PinWindow>> m_recentClosedPins;
    std::vector<QPointer<PinWindow>> m_bulkHiddenPins;
    std::unique_ptr<PinSnapOverlay>  m_snapOverlay;
    int                              m_recoveryLimit{20};
    bool                             m_lastShowShadow{true};
};
