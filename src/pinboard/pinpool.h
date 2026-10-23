#pragma once

#include <QObject>
#include <QPixmap>
#include <QPointer>

#include <vector>

class PinWindow;

class PinPool : public QObject
{
    Q_OBJECT

public:
    explicit PinPool(QObject *parent = nullptr);
    ~PinPool() override;

    void pinImage(const QPixmap &pixmap, const QPoint &position);
    int  pinCount() const;

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
    void removeClosedPin(PinWindow *window);
    void removeNullPins();

    std::vector<QPointer<PinWindow>> m_pinWindows;
    std::vector<QPointer<PinWindow>> m_recentClosedPins;
};
