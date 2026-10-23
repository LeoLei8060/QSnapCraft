#pragma once

#include <QPixmap>
#include <QPoint>
#include <QTransform>
#include <QWidget>

class QAction;
class QCloseEvent;
class QContextMenuEvent;
class QGraphicsDropShadowEffect;
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

    void pinImage(const QPixmap &pixmap, const QRect &rect);
    void setClickThrough(bool enabled);
    bool isClickThrough() const;
    void setShadowEnabled(bool enabled);
    void setStayOnTop(bool enabled);

signals:
    void sigCloseRequested(PinWindow *window);

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

private:
    void         createContextMenu();
    void         updateImageLabelGeometry();
    void         copyImage() const;
    void         saveImageAs();
    void         applyTransform(const QTransform &transform);
    void         scaleContentBy(qreal factor);
    void         moveContentBy(const QPoint &delta);
    void         adjustOpacity(int direction);
    ResizeHandle hitTest(const QPoint &pos) const;
    void         adjustRect(const QPoint &pos);
    void         updateCursor(ResizeHandle handle);

    QLabel                    *m_imageLabel{nullptr};
    QGraphicsDropShadowEffect *m_shadowEffect{nullptr};
    QMenu                     *m_contextMenu{nullptr};
    QAction                   *m_shadowAction{nullptr};
    QPixmap                    m_currentImage;
    QPoint                     m_dragStartPos;
    QRect                      m_startGeometry;
    ResizeHandle               m_currentHandle{ResizeHandle::None};
    bool                       m_isDragging{false};
    bool                       m_clickThrough{false};
    bool                       m_stayOnTop{true};
    static constexpr int       RESIZE_HANDLE_SIZE = 8;
    static constexpr int       SHADOW_MARGIN = 10;
    static constexpr int       MIN_CONTENT_SIZE = 50;
    static constexpr int       MAX_CONTENT_SIZE = 4096;
};
