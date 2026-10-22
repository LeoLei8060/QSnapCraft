#pragma once

#include "ui/rulermeasurement.h"

#include <QColor>
#include <QPoint>
#include <QPointer>
#include <QVector>
#include <QWidget>

class QScreen;
class QToolButton;

class ScreenRulerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenRulerWindow(QWidget *parent = nullptr);
    ~ScreenRulerWindow() override;

    void             showAtCursor();
    void             setOrientation(RulerOrientation orientation);
    RulerOrientation orientation() const;
    void             setUnit(RulerUnit unit);
    RulerUnit        unit() const;
    QString          lastMeasurement() const;

signals:
    void sigClosed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    enum class DragMode { kNone, kMove, kResizeStart, kResizeEnd, kMeasureNear, kMeasureFar };
    enum class FullMeasureMode { kRectangle, kDistance };

    void     createControls();
    void     layoutControls();
    void     showOptionsMenu(const QPoint &global_position);
    void     startFullMeasurement(FullMeasureMode mode);
    void     finishFullMeasurement(const QString &text);
    void     cancelFullMeasurement();
    void     setOpacityPercent(int percent);
    void     setStayOnTop(bool enabled);
    void     clampToScreen();
    DragMode hitTest(const QPoint &position) const;
    int      axisLength() const;
    int      axisCoordinate(const QPoint &position) const;
    void     updateCursor(const QPoint &position);
    QScreen *rulerScreen() const;
    QRect    targetScreenRect() const;

    QWidget               *m_controls{nullptr};
    QVector<QToolButton *> m_controlButtons;
    QPointer<QWidget>      m_measurementOverlay;
    RulerOrientation       m_orientation{RulerOrientation::kVertical};
    RulerUnit              m_unit{RulerUnit::kPixels};
    QColor                 m_background{255, 255, 213};
    QColor                 m_foreground{0, 0, 0};
    QColor                 m_accent{0, 190, 210};
    QPoint                 m_dragStartGlobal;
    QRect                  m_dragStartGeometry;
    DragMode               m_dragMode{DragMode::kNone};
    DragMode               m_hoverMode{DragMode::kNone};
    int                    m_tickStart{0};
    int                    m_tickEnd{0};
    int                    m_opacityPercent{100};
    int                    m_ppiTenths{960};
    bool                   m_hasTickMeasurement{false};
    bool                   m_stayOnTop{true};
    bool                   m_autoCopy{false};
    int                    m_horizontalLength{1000};
    int                    m_verticalLength{800};
    QString                m_lastMeasurement;
};
