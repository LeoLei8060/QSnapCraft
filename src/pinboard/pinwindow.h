#pragma once

#include "config/pinoptions.h"

#include <QLineF>
#include <QPixmap>
#include <QPoint>
#include <QTransform>
#include <QVector>
#include <QWidget>

class QAction;
class QCloseEvent;
class QContextMenuEvent;
class QDragEnterEvent;
class QDropEvent;
class QGraphicsDropShadowEffect;
class QImage;
class QBuffer;
class QMovie;
class QKeyEvent;
class QLabel;
class QMenu;
class QWheelEvent;

class PinWindow : public QWidget
{
    Q_OBJECT

public:
    // 调整手柄的位置枚举
    enum class ResizeHandle { None, TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left, Move };

    explicit PinWindow(QWidget *parent = nullptr);
    ~PinWindow();

    void       pinImage(const QPixmap &pixmap, const QRect &rect, bool showWindow = true);
    void       pinAnimation(const QByteArray &data, const QString &format, const QRect &rect);
    void       setClickThrough(bool enabled);
    bool       isClickThrough() const;
    void       setShadowEnabled(bool enabled);
    void       setStayOnTop(bool enabled);
    void       setSnapDistancePx(int distancePx);
    void       setSnapTargets(const QVector<QRect> &targets);
    void       setSmoothScaling(bool enabled);
    void       setZoomAtCursor(bool enabled);
    QRect      contentRect() const;
    QPixmap    currentImage() const;
    QString    pinId() const;
    QString    groupId() const;
    void       setPinId(const QString &pinId);
    void       setGroup(const QString &groupId);
    void       setGroups(const QVector<PinGroupData> &groups);
    double     opacity() const;
    bool       shadowEnabled() const;
    bool       isAnimated() const;
    QByteArray animationData() const;
    QString    animationFormat() const;

signals:
    void sigCloseRequested(PinWindow *window);
    void sigContentGeometryChanged(PinWindow *window, const QRect &content_rect);
    void sigSnapGuidesChanged(PinWindow *window, const QVector<QLineF> &guides);
    void sigStateChanged(PinWindow *window);
    void sigGroupAssignmentRequested(PinWindow *window, const QString &groupId);
    void sigCreateGroupRequested(PinWindow *window);
    void sigGroupActionRequested(PinWindow *window, const QString &groupId, PinGroupAction action);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void         createContextMenu();
    void         updateImageLabelGeometry();
    void         copyImage() const;
    void         saveImageAs();
    void         applyTransform(const QTransform &transform);
    void         scaleContentBy(qreal factor, const QPoint &anchorGlobal);
    void         moveContentBy(const QPoint &delta);
    void         adjustOpacity(int direction);
    void         setContentGeometry(const QRect &contentRect);
    void         refreshDisplayedPixmap();
    void         replaceImage(const QImage &image);
    void         applyClickThroughStyle();
    void         notifyContentGeometryChanged();
    ResizeHandle hitTest(const QPoint &pos) const;
    void         adjustRect(const QPoint &pos);
    void         updateCursor(ResizeHandle handle);

    QLabel                    *m_imageLabel{nullptr};
    QGraphicsDropShadowEffect *m_shadowEffect{nullptr};
    QMenu                     *m_contextMenu{nullptr};
    QAction                   *m_shadowAction{nullptr};
    QAction                   *m_animationAction{nullptr};
    QMenu                     *m_animationMenu{nullptr};
    QMenu                     *m_groupMenu{nullptr};
    QVector<PinGroupData>      m_groups;
    QString                    m_pinId;
    QString                    m_groupId;
    QBuffer                   *m_animationBuffer{nullptr};
    QMovie                    *m_movie{nullptr};
    QByteArray                 m_animationData;
    QString                    m_animationFormat;
    QPixmap                    m_currentImage;
    QPoint                     m_dragStartPos;
    QRect                      m_startGeometry;
    QRect                      m_reportedContentRect;
    QVector<QRect>             m_snapTargets;
    QSize                      m_displayedSize;
    qreal                      m_displayedDevicePixelRatio{1.0};
    ResizeHandle               m_currentHandle{ResizeHandle::None};
    bool                       m_isDragging{false};
    bool                       m_clickThrough{false};
    bool                       m_stayOnTop{true};
    int                        m_snapDistancePx{8};
    bool                       m_smoothScaling{true};
    bool                       m_zoomAtCursor{true};
    static constexpr int       RESIZE_HANDLE_SIZE = 8;
    static constexpr int       SHADOW_MARGIN = 10;
    static constexpr int       MIN_CONTENT_SIZE = 50;
    static constexpr int       MAX_CONTENT_SIZE = 4096;
};
