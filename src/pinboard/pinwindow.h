#pragma once

#include <QPoint>
#include <QWidget>

class PinWindow : public QWidget
{
    Q_OBJECT

public:
    // 调整手柄的位置枚举
    enum class ResizeHandle {
        None,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left,
        Move
    };

    explicit PinWindow(QWidget *parent = nullptr);
    ~PinWindow();

    void pinImage(const QPixmap &pixmap, const QRect &rect);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    ResizeHandle hitTest(const QPoint &pos) const;
    void         adjustRect(const QPoint &pos);
    void         updateCursor(ResizeHandle handle);
    QSize        calculateFitSize(const QSize &imageSize, const QSize &maxSize) const;

    QPixmap                m_currentImage;
    QPixmap                m_scaledImage;
    QPoint                 m_dragStartPos;
    QRect                  m_startGeometry;
    ResizeHandle           m_currentHandle{ResizeHandle::None};
    bool                   m_isDragging{false};
    qreal                  m_scaleFactor{1.0};
    static constexpr qreal MIN_SCALE{0.1};
    static constexpr qreal MAX_SCALE{5.0};
    static constexpr int   RESIZE_HANDLE_SIZE = 8; // 调整手柄的大小
};
